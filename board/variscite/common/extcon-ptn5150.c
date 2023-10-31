// SPDX-License-Identifier: GPL-2.0+
/**
 * Driver for NXP PTN5150 CC LOGIC USB EXTCON support
 *
 * Copyright 2023 Variscite Ltd.
 * Author: Nate Drude <nate.d@variscite.com>
 */
#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <linux/bitfield.h>
#include "extcon-ptn5150.h"

int extcon_ptn5150_setup(struct extcon_ptn5150 *port)
{
	int ret = extcon_ptn5150_parse_fdt(port);
	if (ret) {
		printf("%s: Failed to parse device tree\n", __func__);
		return ret;
	}

	ret = extcon_ptn5150_init(port);
	if (ret) {
		printf("%s: port init failed, err=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

/* TODO: Support multiple ptn5150s by iterating nxp,ptn5150 compatible nodes */
int extcon_ptn5150_parse_fdt(struct extcon_ptn5150 *port) {
	int ptn5150_node;

	memset(port, 0, sizeof(struct extcon_ptn5150));

	ptn5150_node = fdt_node_offset_by_compatible(gd->fdt_blob, 0, "nxp,ptn5150");
	if (ptn5150_node < 0) {
		printf("%s: failed to find node, err=%d\n", __func__, ptn5150_node);
		return -ENODEV;
	}

	port->i2c_cfg.addr = fdtdec_get_uint(gd->fdt_blob, ptn5150_node, "reg", -1);
	if (port->i2c_cfg.addr < 0) {
		printf("%s: failed to find reg, err=%d\n", __func__, port->i2c_cfg.addr);
		return -ENODEV;
	}

	port->i2c_cfg.bus = fdtdec_get_uint(gd->fdt_blob, ptn5150_node, "i2c-bus", -1);
	if (port->i2c_cfg.bus < 0) {
		printf("%s: failed to find i2c-bus, err=%d\n", __func__, port->i2c_cfg.bus);
		return -ENODEV;
	}

	return 0;
}

int extcon_ptn5150_phy_mode(struct extcon_ptn5150 *port) {
	int cc_status = extcon_ptn5150_cc_status(port);
	int usb_phy_mode;

	switch (cc_status) {
	case PTN5150_CC_STATUS_DFP:
		printf("%s: phy mode is device\n", __func__);
		usb_phy_mode = USB_INIT_DEVICE;
		break;
	case PTN5150_CC_STATUS_UFP:
		printf("%s: phy mode is host\n", __func__);
		usb_phy_mode = USB_INIT_HOST;
		break;
	default:
		usb_phy_mode = -ENODEV;
		break;
	}

	return usb_phy_mode;
}

int extcon_ptn5150_cc_status(struct extcon_ptn5150 *port) {
	uint8_t cc_status_reg;
	int ret;

	if (!port->i2c_dev)
		return -ENODEV;

	/* Read Control Register */
	ret = dm_i2c_read(port->i2c_dev, PTN5150_CC_STATUS_REG, (uint8_t *)&cc_status_reg, 1);
	if (ret) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}

	return FIELD_GET(PTN5150_CC_STATUS_ATTACH_MASK, cc_status_reg);
}

int extcon_ptn5150_init(struct extcon_ptn5150 *port)
{
	int ret;
	uint8_t vendor_reg;
	struct udevice *bus;
	struct udevice *i2c_dev = NULL;

	if (port == NULL)
		return -EINVAL;

	ret = uclass_get_device_by_seq(UCLASS_I2C, port->i2c_cfg.bus, &bus);
	if (ret) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	ret = dm_i2c_probe(bus, port->i2c_cfg.addr, 0, &i2c_dev);
	if (ret) {
		printf("%s: Can't find device id=0x%x\n",
			__func__, port->i2c_cfg.addr);
		return -ENODEV;
	}

	port->i2c_dev = i2c_dev;

	/* Read Vendor ID and Version ID */
	ret = dm_i2c_read(port->i2c_dev, PTN5150_ID_REG, (uint8_t *)&vendor_reg, 1);
	if (ret) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}

	printf("PTN5150: Vendor ID [0x%lx], Version ID [0x%lx], Addr [I2C%u 0x%x]\n",
		FIELD_GET(PTN5150_ID_VENDOR_MASK, vendor_reg),
		FIELD_GET(PTN5150_ID_VERSION_MASK, vendor_reg),
		port->i2c_cfg.bus, port->i2c_cfg.addr);

	return 0;
}
