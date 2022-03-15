/*
 * Copyright 2020-2021 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <net.h>
#include <miiphy.h>
#include <env.h>

#include "../common/imx8_eeprom.h"

#define AR803x_PHY_ID_1			0x4d
#define ADIN1300_PHY_ID_1		0x283
#define ADIN1300_EXT_REG_PTR		0x10
#define ADIN1300_EXT_REG_DATA		0x11
#define ADIN1300_GE_RGMII_CFG		0xff23

int board_phy_config(struct phy_device *phydev)
{
	u32 phy_id_1;

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	phy_id_1 = (phydev->phy_id >> 16);

	/* Use mii register 0x2 to determine if AR8033 or ADIN1300 */
	switch(phy_id_1) {
	case AR803x_PHY_ID_1:
		printf("AR8033 PHY detected at addr %d\n", phydev->addr);
		break;
	case ADIN1300_PHY_ID_1:
		printf("ADIN1300 PHY detected at addr %d\n", phydev->addr);
		/* ADIN1300 Disable RGMII RX clock delay (enabled by default) */
		phy_write(phydev, MDIO_DEVAD_NONE, ADIN1300_EXT_REG_PTR, ADIN1300_GE_RGMII_CFG);
		phy_write(phydev, MDIO_DEVAD_NONE, ADIN1300_EXT_REG_DATA, 0xe01);
	break;
	default:
		printf("%s: unknown phy_id 0x%x at addr %d\n", __func__, phy_id_1, phydev->addr);
		break;
	}

	return 0;
}

#if defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMX8MP)

#define CHAR_BIT 8

static uint64_t mac2int(const uint8_t hwaddr[])
{
	int8_t i;
	uint64_t ret = 0;
	const uint8_t *p = hwaddr;

	for (i = 5; i >= 0; i--) {
		ret |= (uint64_t)*p++ << (CHAR_BIT * i);
	}

	return ret;
}

static void int2mac(const uint64_t mac, uint8_t *hwaddr)
{
	int8_t i;
	uint8_t *p = hwaddr;

	for (i = 5; i >= 0; i--) {
		*p++ = mac >> (CHAR_BIT * i);
	}
}
#endif

int var_setup_mac(struct var_eeprom *eeprom)
{
	int ret;
	uint8_t enetaddr[6];

#if defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMX8MP)
	uint64_t addr;
	uint8_t enet1addr[6];
#endif

	ret = eth_env_get_enetaddr("ethaddr", enetaddr);
	if (ret)
		return 0;

	ret = var_eeprom_get_mac(eeprom, enetaddr);
	if (ret)
		return ret;

	if (!is_valid_ethaddr(enetaddr))
		return -1;

	eth_env_set_enetaddr("ethaddr", enetaddr);

#if defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMX8MP)
	addr = mac2int(enetaddr);
	int2mac(addr + 1, enet1addr);
	eth_env_set_enetaddr("eth1addr", enet1addr);
#endif

	return 0;
}
