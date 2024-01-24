// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright 2023 NXP
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <errno.h>
#include <generic-phy.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <clk.h>
#include <regmap.h>
#include <syscon.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>

#define SPARE_IN(n)		(((n) & 0x7) << 25)
#define SPARE_IN_MASK		0xe000000
#define TEST_RANDOM_NUM_EN	BIT(24)
#define TEST_MUX_SRC(n)		(((n) & 0x3) << 22)
#define TEST_MUX_SRC_MASK	0xc00000
#define TEST_EN			BIT(21)
#define TEST_DIV4_EN		BIT(20)
#define VBG_ADJ(n)		(((n) & 0x7) << 17)
#define VBG_ADJ_MASK		0xe0000
#define SLEW_ADJ(n)		(((n) & 0x7) << 14)
#define SLEW_ADJ_MASK		0x1c000
#define CC_ADJ(n)		(((n) & 0x7) << 11)
#define CC_ADJ_MASK		0x3800
#define CM_ADJ(n)		(((n) & 0x7) << 8)
#define CM_ADJ_MASK		0x700
#define PRE_EMPH_ADJ(n)		(((n) & 0x7) << 5)
#define PRE_EMPH_ADJ_MASK	0xe0
#define PRE_EMPH_EN		BIT(4)
#define HS_EN			BIT(3)
#define BG_EN			BIT(2)
#define DISABLE_LVDS		BIT(1)
#define CH_EN(id)		BIT(id)

enum imx8mp_lvds_phy_devtype {
	FSL_LVDS_PHY_IMX8MP,
	FSL_LVDS_PHY_IMX93,
	FSL_LVDS0_PHY_IMX95,
	FSL_LVDS1_PHY_IMX95,
};

struct imx8mp_lvds_phy_devdata {
	u32 lvds_ctrl;
	bool has_disable;
};

static const struct imx8mp_lvds_phy_devdata imx8mp_lvds_phy_devdata[] = {
	[FSL_LVDS_PHY_IMX8MP] = {
		.lvds_ctrl = 0x128,
		.has_disable = false,
	},
	[FSL_LVDS_PHY_IMX93] = {
		.lvds_ctrl = 0x24,
		.has_disable = true,
	},
	[FSL_LVDS0_PHY_IMX95] = {
		.lvds_ctrl = 0x8,
		.has_disable = true,
	},
	[FSL_LVDS1_PHY_IMX95] = {
		.lvds_ctrl = 0xc,
		.has_disable = true,
	},
};

struct imx8mp_lvds_phy_priv {
	struct udevice *dev;
	struct regmap *regmap;
	struct clk apb_clk;
	const struct imx8mp_lvds_phy_devdata *devdata;
};

static inline unsigned int phy_read(struct phy *phy, unsigned int reg)
{
	struct imx8mp_lvds_phy_priv *priv = dev_get_priv(dev_get_parent(phy->dev));
	unsigned int val;

	regmap_read(priv->regmap, reg, &val);

	return val;
}

static inline void
phy_write(struct phy *phy, unsigned int reg, unsigned int value)
{
	struct imx8mp_lvds_phy_priv *priv = dev_get_priv(dev_get_parent(phy->dev));

	regmap_write(priv->regmap, reg, value);
}

static int imx8mp_lvds_phy_init(struct phy *phy)
{
	struct imx8mp_lvds_phy_priv *priv;
	ulong drv_data = dev_get_driver_data(phy->dev);

	/* Call from parent phy node should be invalid */
	if (drv_data)
		return -EINVAL;

	priv = dev_get_priv(dev_get_parent(phy->dev));

	phy_write(phy, priv->devdata->lvds_ctrl,
			CC_ADJ(0x2) | PRE_EMPH_EN | PRE_EMPH_ADJ(0x3));

	return 0;
}

static int imx8mp_lvds_phy_power_on(struct phy *phy)
{
	struct imx8mp_lvds_phy_priv *priv;
	ulong drv_data = dev_get_driver_data(phy->dev);
	u32 id, val;
	bool bg_en;

	/* Call from parent phy node should be invalid */
	if (drv_data)
		return -EINVAL;

	id = (ulong)dev_get_plat(phy->dev);
	priv = dev_get_priv(dev_get_parent(phy->dev));

	val = phy_read(phy, priv->devdata->lvds_ctrl);
	bg_en = !!(val & BG_EN);
	val |= BG_EN;
	if (priv->devdata->has_disable)
		val &= ~DISABLE_LVDS;
	phy_write(phy, priv->devdata->lvds_ctrl, val);

	/* Wait 15us to make sure the bandgap to be stable. */
	if (!bg_en)
		udelay(20);

	val = phy_read(phy, priv->devdata->lvds_ctrl);
	val |= CH_EN(id);
	phy_write(phy, priv->devdata->lvds_ctrl, val);

	/* Wait 5us to ensure the phy be settling. */
	udelay(10);

	return 0;
}

static int imx8mp_lvds_phy_power_off(struct phy *phy)
{
	struct imx8mp_lvds_phy_priv *priv;
	ulong drv_data = dev_get_driver_data(phy->dev);
	u32 id, val;

	/* Call from parent phy node should be invalid */
	if (drv_data)
		return -EINVAL;

	id = (ulong)dev_get_plat(phy->dev);
	priv = dev_get_priv(dev_get_parent(phy->dev));

	val = phy_read(phy, priv->devdata->lvds_ctrl);
	val &= ~BG_EN;
	phy_write(phy, priv->devdata->lvds_ctrl, val);

	val = phy_read(phy, priv->devdata->lvds_ctrl);
	val &= ~CH_EN(id);
	if (priv->devdata->has_disable)
		val |= DISABLE_LVDS;
	phy_write(phy, priv->devdata->lvds_ctrl, val);

	return 0;
}

static const struct phy_ops imx8mp_lvds_phy_ops = {
	.init = imx8mp_lvds_phy_init,
	.power_on = imx8mp_lvds_phy_power_on,
	.power_off = imx8mp_lvds_phy_power_off,
};

static int imx8mp_lvds_phy_probe(struct udevice *dev)
{
	struct imx8mp_lvds_phy_priv *priv = dev_get_priv(dev);
	int ret;

	priv->devdata = (const struct imx8mp_lvds_phy_devdata *)dev_get_driver_data(dev);
	if (!priv->devdata) /* sub phy node device, directly return */
		return 0;

	priv->regmap = syscon_regmap_lookup_by_phandle(dev, "gpr");
	if (IS_ERR(priv->regmap)) {
		dev_err(dev, "failed to get regmap\n");
		return PTR_ERR(priv->regmap);
	}

	priv->dev = dev;

	ret = clk_get_by_name(dev, "apb", &priv->apb_clk);
	if (ret) {
		dev_err(dev, "cannot get apb clock\n");
		return ret;
	}

	clk_enable(&priv->apb_clk);

	return 0;
}

static int imx8mp_lvds_phy_remove(struct udevice *dev)
{
	return 0;
}

static int imx8mp_lvds_phy_bind(struct udevice *dev)
{
	ofnode child;
	u32 phy_id;
	ulong drv_data = dev_get_driver_data(dev);
	int ret = 0;

	if (drv_data) {
		/* Parent lvds phy node, bind each subnode to driver */
		ofnode_for_each_subnode(child, dev_ofnode(dev)) {
			if (ofnode_read_u32(child, "reg", &phy_id)) {
				dev_err(dev, "missing reg property in node %s\n",
					ofnode_get_name(child));
				return -EINVAL;
			}

			if (phy_id >= 2) {
				dev_err(dev, "invalid reg in node %s\n", ofnode_get_name(child));
				return -EINVAL;
			}

			ret = device_bind(dev, dev->driver, "ldb_phy", (void *)(ulong)phy_id,
				child, NULL);
			if (ret)
				printf("Error binding driver '%s': %d\n", dev->driver->name,
					ret);
		}
	}

	return ret;
}

static const struct udevice_id imx8mp_lvds_phy_of_match[] = {
	{ .compatible = "fsl,imx8mp-lvds-phy",
	  .data = (ulong)&imx8mp_lvds_phy_devdata[FSL_LVDS_PHY_IMX8MP] },
	{ .compatible = "fsl,imx93-lvds-phy",
	  .data = (ulong)&imx8mp_lvds_phy_devdata[FSL_LVDS_PHY_IMX93] },
	{ .compatible = "fsl,imx95-lvds0-phy",
	  .data = (ulong)&imx8mp_lvds_phy_devdata[FSL_LVDS0_PHY_IMX95] },
	{ .compatible = "fsl,imx95-lvds1-phy",
	  .data = (ulong)&imx8mp_lvds_phy_devdata[FSL_LVDS1_PHY_IMX95] },
	{}
};

U_BOOT_DRIVER(imx8mp_lvds_phy) = {
	.name = "imx8mp_lvds_phy",
	.id = UCLASS_PHY,
	.of_match = imx8mp_lvds_phy_of_match,
	.probe = imx8mp_lvds_phy_probe,
	.remove = imx8mp_lvds_phy_remove,
	.bind = imx8mp_lvds_phy_bind,
	.ops = &imx8mp_lvds_phy_ops,
	.priv_auto	= sizeof(struct imx8mp_lvds_phy_priv),
};
