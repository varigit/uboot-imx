// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright 2023 NXP
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <video.h>
#include <video_bridge.h>
#include <video_link.h>
#include <asm/io.h>
#include <dm/device-internal.h>
#include <dm/ofnode.h>
#include <linux/iopoll.h>
#include <linux/err.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <regmap.h>
#include <syscon.h>
#include <display.h>

#define CTRL		0x8
#define  PL_VALID(n)	BIT(1 + 4 * (n))
#define  PL_ENABLE(n)	BIT(4 * (n))

#define STREAMS		2
#define OUT_ENDPOINTS	2

struct imx95_pl {
	struct regmap *regmap;
	u32 pl_id;
	struct udevice *next_bridge;
	struct display_timing adj;
};

static void imx95_pl_bridge_disable(struct imx95_pl *pl)
{
	unsigned int id = pl->pl_id;

	regmap_update_bits(pl->regmap, CTRL, PL_ENABLE(id), 0);
	regmap_update_bits(pl->regmap, CTRL, PL_VALID(id), 0);
}

static void imx95_pl_bridge_enable(struct imx95_pl *pl)
{
	unsigned int id = pl->pl_id;

	regmap_update_bits(pl->regmap, CTRL, PL_VALID(id), PL_VALID(id));
	regmap_update_bits(pl->regmap, CTRL, PL_ENABLE(id), PL_ENABLE(id));
}

static int imx95_pl_bridge_attach(struct udevice *dev)
{
	struct imx95_pl *pl = dev_get_priv(dev);
	struct display_timing timings;
	int ret;
	ofnode endpoint, port_node;

	debug("%s\n", __func__);

	/* Curretly only support 1 pixel link in u-boot */
	pl->next_bridge = video_link_get_next_device(dev);
	if (!pl->next_bridge ||
		(device_get_uclass_id(pl->next_bridge) != UCLASS_VIDEO_BRIDGE
		&& device_get_uclass_id(pl->next_bridge) != UCLASS_DISPLAY)) {
		dev_err(dev, "get pl bridge device error\n");
		return -ENODEV;
	}

	endpoint = video_link_get_ep_to_nextdev(pl->next_bridge);
	if (!ofnode_valid(endpoint)) {
		dev_err(dev, "fail to get endpoint to next bridge\n");
		return -ENODEV;
	}

	port_node = ofnode_get_parent(endpoint);

	/* First two ports for input, last two ports for output */
	pl->pl_id = (u32)ofnode_get_addr_size_index_notrans(port_node, 0, NULL);
	pl->pl_id -= STREAMS;

	ret = video_link_get_display_timings(&timings);
	if (ret) {
		dev_err(dev, "decode display timing error %d\n", ret);
		return ret;
	}

	pl->adj = timings;

	if (device_get_uclass_id(pl->next_bridge) == UCLASS_VIDEO_BRIDGE) {
		ret = video_bridge_attach(pl->next_bridge);
		if (ret) {
			dev_err(dev, "fail to attach pixel link device\n");
			return ret;
		}
	}

	return 0;
}

static int imx95_pl_check_timing(struct udevice *dev, struct display_timing *timing)
{
	struct imx95_pl *pl = dev_get_priv(dev);

	/* Ensure the bridge device attached with next bridge */
	if (!pl->next_bridge) {
		dev_err(dev, "%s No next bridge device attached\n", __func__);
		return -ENOTCONN;
	}

	*timing = pl->adj;

	return 0;
}

static int imx95_pl_set_backlight(struct udevice *dev, int percent)
{
	struct imx95_pl *pl = dev_get_priv(dev);
	int ret;

	imx95_pl_bridge_enable(pl);

	if (device_get_uclass_id(pl->next_bridge) == UCLASS_VIDEO_BRIDGE) {
		ret = video_bridge_set_backlight(pl->next_bridge, percent);
		if (ret) {
			dev_err(dev, "fail to set backlight\n");
			return ret;
		}
	} else if (device_get_uclass_id(pl->next_bridge) == UCLASS_DISPLAY) {
		ret = display_enable(pl->next_bridge, 24, NULL);
		if (ret) {
			dev_err(dev, "fail to enable display\n");
			return ret;
		}
	}

	return 0;
}

static const struct video_bridge_ops imx95_pl_bridge_ops = {
	.attach			= imx95_pl_bridge_attach,
	.check_timing 		= imx95_pl_check_timing,
	.set_backlight 		= imx95_pl_set_backlight,
};

static int imx95_pl_probe(struct udevice *dev)
{
	struct imx95_pl *pl = dev_get_priv(dev);

	pl->regmap = syscon_node_to_regmap(dev_ofnode(dev_get_parent(dev)));
	if (IS_ERR(pl->regmap)) {
		dev_err(dev, "failed to get regmap\n");
		return IS_ERR(pl->regmap);
	}

	return 0;
}

static int imx95_pl_remove(struct udevice *dev)
{
	struct imx95_pl *pl = dev_get_priv(dev);

	if (pl->next_bridge)
		device_remove(pl->next_bridge, DM_REMOVE_NORMAL);

	imx95_pl_bridge_disable(pl);

	return 0;
}

static const struct udevice_id imx95_pl_dt_ids[] = {
	{ .compatible = "nxp,imx95-dc-pixel-link" },
	{ }
};

U_BOOT_DRIVER(imx95_pl_driver) = {
	.name				= "imx95_pl_driver",
	.id				= UCLASS_VIDEO_BRIDGE,
	.of_match			= imx95_pl_dt_ids,
	.remove				= imx95_pl_remove,
	.probe				= imx95_pl_probe,
	.ops				= &imx95_pl_bridge_ops,
	.priv_auto			= sizeof(struct imx95_pl),
};
