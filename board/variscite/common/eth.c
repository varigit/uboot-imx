#include <common.h>
#include <net.h>
#include <miiphy.h>
#include <environment.h>

#include "../common/imx8_eeprom.h"

#define AR803x_PHY_DEBUG_ADDR_REG	0x1d
#define AR803x_PHY_DEBUG_DATA_REG	0x1e

#define AR803x_DEBUG_REG_5		0x05
#define AR803x_DEBUG_REG_0		0x00

#define AR803x_DEBUG_REG_0		0x00
#define AR803x_DEBUG_REG_31		0x1f
#define AR803x_VDDIO_1P8V_EN		0x8

int board_phy_config(struct phy_device *phydev)
{
	/* Disable RGMII RX clock delay (enabled by default) */
	phy_write(phydev, MDIO_DEVAD_NONE, AR803x_PHY_DEBUG_ADDR_REG,
		  AR803x_DEBUG_REG_0);
	phy_write(phydev, MDIO_DEVAD_NONE, AR803x_PHY_DEBUG_DATA_REG, 0);

	/* Enable 1.8V VDDIO voltage */
	phy_write(phydev, MDIO_DEVAD_NONE, AR803x_PHY_DEBUG_ADDR_REG,
		  AR803x_DEBUG_REG_31);
	phy_write(phydev, MDIO_DEVAD_NONE, AR803x_PHY_DEBUG_DATA_REG,
		AR803x_VDDIO_1P8V_EN);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

#define CHAR_BIT 8

#ifdef CONFIG_ARCH_IMX8
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
	unsigned char enetaddr[6];

#ifdef CONFIG_ARCH_IMX8
	uint64_t addr;
	unsigned char enet1addr[6];
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

#ifdef CONFIG_ARCH_IMX8
	addr = mac2int(enetaddr);
	int2mac(addr + 1, enet1addr);
	eth_env_set_enetaddr("eth1addr", enet1addr);
#endif

	return 0;
}
