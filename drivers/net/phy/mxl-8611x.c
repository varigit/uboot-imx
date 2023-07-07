// SPDX-License-Identifier: GPL-2.0+
/**
 *  Driver for MaxLinear MXL861100 Ethernet PHY
 *
 * Copyright 2023 Variscite Ltd.
 * Copyright 2023 MaxLinear Inc.
 * Author: Nate Drude <nate.d@variscite.com>
 */
#include <common.h>
#include <phy.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>

/* PHY IDs */
#define PHY_ID_MXL86110		0xC1335580
#define PHY_ID_MXL86111		0xC1335588

/* required to access extended registers */
#define MXL8611X_EXTD_REG_ADDR_OFFSET				0x1E
#define MXL8611X_EXTD_REG_ADDR_DATA				0x1F

/* RGMII register */
#define MXL8611X_EXT_RGMII_CFG1_REG				0xA003
#define MXL8611X_EXT_RGMII_CFG1_NO_DELAY			0

#define MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK			GENMASK(13, 10)
#define MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK		GENMASK(3, 0)
#define MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK	GENMASK(7, 4)

/* LED registers and defines */
#define MXL8611X_LED0_CFG_REG					0xA00C
#define MXL8611X_LED1_CFG_REG					0xA00D
#define MXL8611X_LED2_CFG_REG					0xA00E

/**
 * struct mxl8611x_cfg_reg_map - map a config value to aregister value
 * @cfg		value in device configuration
 * @reg		value in the register
 */
struct mxl8611x_cfg_reg_map {
	int cfg;
	int reg;
};

static const struct mxl8611x_cfg_reg_map mxl8611x_rgmii_delays[] = {
	{ 0, 0 },
	{ 150, 1 },
	{ 300, 2 },
	{ 450, 3 },
	{ 600, 4 },
	{ 750, 5 },
	{ 900, 6 },
	{ 1050, 7 },
	{ 1200, 8 },
	{ 1350, 9 },
	{ 1500, 10 },
	{ 1650, 11 },
	{ 1800, 12 },
	{ 1950, 13 },
	{ 2100, 14 },
	{ 2250, 15 },
	{ 0, 0 } // Marks the end of the array
};

static int mxl8611x_lookup_reg_value(const struct mxl8611x_cfg_reg_map *tbl,
				     const int cfg, int *reg)
{
	size_t i;

	for (i = 0; i == 0 || tbl[i].cfg; i++) {
		if (tbl[i].cfg == cfg) {
			*reg = tbl[i].reg;
			return 0;
		}
	}

	return -EINVAL;
}

static u16 mxl8611x_ext_read(struct phy_device *phydev, const u32 regnum)
{
	u16 val;

	phy_write(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_OFFSET, regnum);
	val = phy_read(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_DATA);

	debug("%s: MXL86110@0x%x 0x%x=0x%x\n", __func__, phydev->addr, regnum, val);

	return val;
}

static int mxl8611x_ext_write(struct phy_device *phydev, const u32 regnum, const u16 val)
{
	debug("%s: MXL86110@0x%x 0x%x=0x%x\n", __func__, phydev->addr, regnum, val);

	phy_write(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_OFFSET, regnum);

	return phy_write(phydev, MDIO_DEVAD_NONE, MXL8611X_EXTD_REG_ADDR_DATA, val);
}

static int mxl8611x_extread(struct phy_device *phydev, int addr, int devaddr,
			       int regnum)
{
	return mxl8611x_ext_read(phydev, regnum);
}

static int mxl8611x_extwrite(struct phy_device *phydev, int addr,
				int devaddr, int regnum, u16 val)
{
	return mxl8611x_ext_write(phydev, regnum, val);
}

static int mxl8611x_led_cfg(struct phy_device *phydev)
{
	int ret = 0;
	int i;
	char propname[25];
	u32 val;

	ofnode node = phy_get_ofnode(phydev);

	if (!ofnode_valid(node)) {
		printf("%s: failed to get node\n", __func__);
		return -EINVAL;
	}

	/* Loop through three the LED registers */
	for (i = 0; i < 3; i++) {
		/* Read property from device tree */
		ret = snprintf(propname, 25, "mxl-8611x,led%d_cfg", i);
		if (ofnode_read_u32(node, propname, &val))
			continue;

		/* Update PHY LED register */
		mxl8611x_ext_write(phydev, MXL8611X_LED0_CFG_REG + i, val);
	}

	return 0;
}

static int mxl8611x_rgmii_cfg_of_delay(struct phy_device *phydev, const char *property, int *val)
{
	u32 of_val;
	int ret;

	ofnode node = phy_get_ofnode(phydev);

	if (!ofnode_valid(node)) {
		printf("%s: failed to get node\n", __func__);
		return -EINVAL;
	}

	/* Get property from dts */
	ret = ofnode_read_u32(node, property, &of_val);
	if (ret)
		return ret;

	/* Convert delay in ps to register value */
	ret = mxl8611x_lookup_reg_value(mxl8611x_rgmii_delays, of_val, val);
	if (ret)
		printf("%s: Error: %s = %d is invalid, using default value\n",
		       __func__, property, of_val);

	return ret;
}

static int mxl8611x_rgmii_cfg(struct phy_device *phydev)
{
	u32 val = 0;
	int rxdelay, txdelay_100m, txdelay_1g;

	/* Get rgmii register value */
	val = mxl8611x_ext_read(phydev, MXL8611X_EXT_RGMII_CFG1_REG);

	/* Get RGMII Rx Delay Selection from device tree or rgmii register */
	if (mxl8611x_rgmii_cfg_of_delay(phydev, "mxl-8611x,rx-internal-delay-ps", &rxdelay))
		rxdelay = FIELD_GET(MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK, val);

	/* Get Fast Ethernet RGMII Tx Clock Delay Selection from device tree or rgmii register */
	if (mxl8611x_rgmii_cfg_of_delay(phydev, "mxl-8611x,tx-internal-delay-ps-100m",
					&txdelay_100m))
		txdelay_100m = FIELD_GET(MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK, val);

	/* Get Gigabit Ethernet RGMII Tx Clock Delay Selection from device tree or rgmii register */
	if (mxl8611x_rgmii_cfg_of_delay(phydev, "mxl-8611x,tx-internal-delay-ps-1g", &txdelay_1g))
		txdelay_1g = FIELD_GET(MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK, val);

	switch (phydev->interface) {
	case PHY_INTERFACE_MODE_RGMII:
		val = MXL8611X_EXT_RGMII_CFG1_NO_DELAY;
		break;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK, rxdelay);
		break;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK, txdelay_100m);
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK, txdelay_1g);
		break;
	case PHY_INTERFACE_MODE_RGMII_ID:
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_RX_DELAY_MASK, rxdelay);
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_10MB_100MB_DELAY_MASK, txdelay_100m);
		val = (val & ~MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK) |
			FIELD_PREP(MXL8611X_EXT_RGMII_CFG1_TX_1G_DELAY_MASK, txdelay_1g);
		break;
	default:
		printf("%s: Error: Unsupported rgmii mode %d\n", __func__, phydev->interface);
		return -EINVAL;
	}

	return mxl8611x_ext_write(phydev, MXL8611X_EXT_RGMII_CFG1_REG, val);
}

static int mxl8611x_config(struct phy_device *phydev)
{
	int ret;

	/* Configure rgmii */
	ret = mxl8611x_rgmii_cfg(phydev);

	if (ret < 0)
		return ret;

	/* Configure LEDs */
	ret = mxl8611x_led_cfg(phydev);

	if (ret < 0)
		return ret;

	return genphy_config(phydev);
}

static int mxl86110_config(struct phy_device *phydev)
{
	printf("MXL86110 PHY detected at addr %d\n", phydev->addr);
	return mxl8611x_config(phydev);
}

static int mxl86111_config(struct phy_device *phydev)
{
	printf("MXL86111 PHY detected at addr %d\n", phydev->addr);
	return mxl8611x_config(phydev);
}

static struct phy_driver mxl86110_driver = {
	.name = "MXL86110",
	.uid = PHY_ID_MXL86110,
	.mask = 0xffffffff,
	.features = PHY_GBIT_FEATURES,
	.config = mxl86110_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
	.readext = mxl8611x_extread,
	.writeext = mxl8611x_extwrite,
};

static struct phy_driver mxl86111_driver = {
	.name = "MXL86111",
	.uid = PHY_ID_MXL86111,
	.mask = 0xffffffff,
	.features = PHY_GBIT_FEATURES,
	.config = mxl86111_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
	.readext = mxl8611x_extread,
	.writeext = mxl8611x_extwrite,
};

int phy_mxl8611x_init(void)
{
	phy_register(&mxl86110_driver);
	phy_register(&mxl86111_driver);

	return 0;
}
