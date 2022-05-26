// SPDX-License-Identifier: GPL-2.0+
/**
 * Driver for NXP PTN5150 CC LOGIC USB EXTCON support
 *
 * Copyright 2023 Variscite Ltd.
 * Author: Nate Drude <nate.d@variscite.com>
 */
#ifndef __EXTCON_PTN5150_H
#define __EXTCON_PTN5150_H

#include <dm.h>
#include <linux/bitfield.h>
#include <usb.h>


#define PTN5150_ID_REG				0x1
#define   PTN5150_ID_VENDOR_MASK		GENMASK(2,0)
#define   PTN5150_ID_VERSION_MASK		GENMASK(7,3)

#define PTN5150_CC_STATUS_REG			0x4
#define   PTN5150_CC_STATUS_ATTACH_MASK		GENMASK(4,2)
#define   PTN5150_CC_STATUS_NOT_CONNECTED	0x0
#define   PTN5150_CC_STATUS_DFP			0x1
#define   PTN5150_CC_STATUS_UFP			0x2

struct extcon_ptn5150_i2c_cfg {
	int bus;
	int addr;
};

struct extcon_ptn5150 {
	struct extcon_ptn5150_i2c_cfg i2c_cfg;
	struct udevice *i2c_dev;
};

int extcon_ptn5150_setup(struct extcon_ptn5150 *port);
int extcon_ptn5150_parse_fdt(struct extcon_ptn5150 *port);
int extcon_ptn5150_phy_mode(struct extcon_ptn5150 *port);
int extcon_ptn5150_cc_status(struct extcon_ptn5150 *port);
int extcon_ptn5150_init(struct extcon_ptn5150 *port);

#endif /* __EXTCON_PTN5150_H */
