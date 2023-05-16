#include <common.h>
#include <net.h>
#include <miiphy.h>
#include <env.h>

#if defined(CONFIG_IMX93)
#include "../common/imx9_eeprom.h"
#else
#include "../common/imx8_eeprom.h"
#endif

#define CHAR_BIT 8

#if defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMX8MP) || defined(CONFIG_IMX93)
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
	uint8_t enetaddr[ARP_HLEN];

#if defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMX8MP) || defined(CONFIG_IMX93)
	uint64_t addr;
	uint8_t enet1addr[ARP_HLEN];
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

#if defined(CONFIG_ARCH_IMX8) || defined(CONFIG_IMX8MP) || defined(CONFIG_IMX93)
	addr = mac2int(enetaddr);
	int2mac(addr + 1, enet1addr);
	eth_env_set_enetaddr("eth1addr", enet1addr);
#endif

	return 0;
}
