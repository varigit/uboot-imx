// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 * Copyright 2023 Variscite Ltd.
 */

#include <common.h>
#include <env.h>
#include <init.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/global_data.h>
#include <asm/arch-imx9/ccm_regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch-imx9/imx93_pins.h>
#include <asm/arch/clock.h>
#include <power/pmic.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <usb.h>
#include <dwc3-uboot.h>

#include "../common/imx9_eeprom.h"
#include "../common/extcon-ptn5150.h"

DECLARE_GLOBAL_DATA_PTR;

#define CARRIER_EEPROM_ADDR 0x54

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX93_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX93_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int var_setup_mac(struct var_eeprom *eeprom);

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(LPUART1_CLK_ROOT);

	return 0;
}

int board_phys_sdram_size(phys_size_t *size)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	var_eeprom_get_dram_size(ep, size);
	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

static int setup_eqos(void)
{
	struct blk_ctrl_wakeupmix_regs *bctrl =
		(struct blk_ctrl_wakeupmix_regs *)BLK_CTRL_WAKEUPMIX_BASE_ADDR;

	/* set INTF as RGMII, enable RGMII TXC clock */
	clrsetbits_le32(&bctrl->eqos_gpr,
			BCTRL_GPR_ENET_QOS_INTF_MODE_MASK,
			BCTRL_GPR_ENET_QOS_INTF_SEL_RGMII | BCTRL_GPR_ENET_QOS_CLK_GEN_EN);

	return set_clk_eqos(ENET_125MHZ);
}

#ifdef CONFIG_EXTCON_PTN5150
static struct extcon_ptn5150 usb_ptn5150;
int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int usb_phy_mode = extcon_ptn5150_phy_mode(&usb_ptn5150);

	/* Default to host mode if not connected */
	if (usb_phy_mode < 0) {
		printf("Defaulting to USB Host");
		usb_phy_mode = USB_INIT_HOST;
	}

	return usb_phy_mode;
}
#endif

int board_init(void)
{
	if (CONFIG_IS_ENABLED(DWC_ETH_QOS))
		setup_eqos();

	return 0;
}

#define SDRAM_SIZE_STR_LEN 5

int board_late_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	struct var_carrier_eeprom carrier_eeprom;
	char carrier_rev[CARRIER_REV_LEN] = {0};
	char som_rev[CARRIER_REV_LEN] = {0};

	if (CONFIG_IS_ENABLED(EXTCON_PTN5150)) {
		extcon_ptn5150_setup(&usb_ptn5150);
	}

	var_setup_mac(ep);
	var_eeprom_print_prod_info(ep);

	/* ENV Variables */

	/* SDRAM ENV */
	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d",
		(int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	/* Carrier Board ENV */
	var_carrier_eeprom_read(VAR_CARRIER_EEPROM_I2C_NAME, CARRIER_EEPROM_ADDR, &carrier_eeprom);
	var_carrier_eeprom_get_revision(&carrier_eeprom, carrier_rev, sizeof(carrier_rev));
	env_set("carrier_rev", carrier_rev);

	/* SoM Features */
	if (ep->features & VAR_EEPROM_F_WBE)
		env_set("som_has_wbe", "1");
	else
		env_set("som_has_wbe", "0");

	/* SoM Rev ENV */
	snprintf(som_rev, CARRIER_REV_LEN, "%ld.%ld", SOMREV_MAJOR(ep->somrev), SOMREV_MINOR(ep->somrev));
	env_set("som_rev", som_rev);

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "VAR-SOM-MX93");
#endif
	return 0;
}
