// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 NXP
 * Copyright 2023-2025 Variscite Ltd.
 */

#include <common.h>
#include <env.h>
#include <efi_loader.h>
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
#include "imx93_var_som.h"

DECLARE_GLOBAL_DATA_PTR;

#define SOM_REV_STR_LEN 16

#define CARRIER_EEPROM_ADDR 0x54

#define UART_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_FSEL2)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE(6) | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
	MX93_PAD_UART1_RXD__LPUART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX93_PAD_UART1_TXD__LPUART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int var_setup_mac(struct var_eeprom *eeprom);

int get_board_id(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;

	if (!var_eeprom_is_valid(ep)) {
		printf("%s: assuming VAR_SOM_MX93\n", __func__);
		return VAR_SOM_MX93;
	}

	if (htons(ep->magic) == VAR_DART_EEPROM_MAGIC)
		return DART_MX93;

	return VAR_SOM_MX93;
}

#if defined(CONFIG_MULTI_DTB_FIT) && !defined(CONFIG_SPL_BUILD)
int board_fit_config_name_match(const char *name)
{
	int board_id = get_board_id();

	switch (board_id) {
	case DART_MX93:
		if (!strcmp(name, "imx93-var-dart-dt8mcustomboard"))
			return 0;
		break;
	case VAR_SOM_MX93:
		if (!strcmp(name, "imx93-var-som-symphony"))
			return 0;
		break;
	default:
		return -1;
	}

	return -1;
}
#endif

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0xbc550d86, 0xda26, 0x4b70, 0xac, 0x05, \
	0x2a, 0x44, 0x8e, 0xda, 0x6f, 0x21)

struct efi_fw_image fw_images[] = {
	{
	.image_type_id = IMX_BOOT_IMAGE_GUID,
	.fw_name = u"IMX93-11X11-EVK-RAW",
	.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=flash-bin raw 0 0x2000 mmcpart 1",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

int board_early_init_f(void)
{
	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

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
	return 0;
}

#define SDRAM_SIZE_STR_LEN 5

int board_late_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	int id = get_board_id();
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	struct var_carrier_eeprom carrier_eeprom;
	char carrier_rev[CARRIER_REV_LEN] = {0};
	char som_rev[SOM_REV_STR_LEN] = {0};

#ifdef CONFIG_EXTCON_PTN5150
	if (id == VAR_SOM_MX93)
		extcon_ptn5150_setup(&usb_ptn5150);
#endif

	var_setup_mac(ep);
	var_eeprom_print_prod_info(ep);

	/* ENV Variables */

	/* SDRAM ENV */
	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d",
		 (int)(gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	/* Carrier Board ENV */
	var_carrier_eeprom_read(VAR_CARRIER_EEPROM_I2C_NAME, CARRIER_EEPROM_ADDR, &carrier_eeprom);
	var_carrier_eeprom_get_revision(&carrier_eeprom, carrier_rev, sizeof(carrier_rev));
	env_set("carrier_rev", carrier_rev);

	if (!strncmp(carrier_rev, "dt8m", 4))
		env_set("carrier_name", "dt8mcustomboard");
	else if (!strncmp(carrier_rev, "sym-2", 5))
		env_set("carrier_name", "symphony");
	else if (!strncmp(carrier_rev, "sym-1", 5))
		env_set("carrier_name", "symphony-1.x");
	else if (!strncmp(carrier_rev, "sonata", 6))
		env_set("carrier_name", "sonata");
	else
		env_set("carrier_name", "undefined");

	/* SoM Features */
	if (ep->features & VAR_EEPROM_F_WBE)
		env_set("som_has_wbe", "1");
	else
		env_set("som_has_wbe", "0");

	/* SoM Rev ENV */
	snprintf(som_rev, sizeof(som_rev), "%ld.%ld",
		 SOMREV_MAJOR(ep->somrev), SOMREV_MINOR(ep->somrev));
	env_set("som_rev", som_rev);

#ifdef CONFIG_MMC
	board_late_mmc_env_init();
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	switch (id) {
	case VAR_SOM_MX93:
		env_set("board_name", "VAR-SOM-MX93");
		break;
	case DART_MX93:
		env_set("board_name", "DART-MX93");
		break;
	default:
		env_set("board_name", "UNKNOWN");
		break;
	}
#endif
	return 0;
}

int checkboard(void)
{
	if (get_board_id() == DART_MX93) {
		struct var_carrier_eeprom carrier_eeprom;
		char carrier_rev[CARRIER_REV_LEN] = {0};

		var_carrier_eeprom_read(VAR_CARRIER_EEPROM_I2C_NAME, CARRIER_EEPROM_ADDR, &carrier_eeprom);
		var_carrier_eeprom_get_revision(&carrier_eeprom, carrier_rev, sizeof(carrier_rev));

		if (!strncmp(carrier_rev, "sonata", 6))
			printf("Board: Sonata-Board\n");
		else
			printf("Board: DT8MCustomBoard\n");
	}

	return 0;
}
