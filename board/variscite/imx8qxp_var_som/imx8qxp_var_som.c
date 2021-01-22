// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018 NXP
 * Copyright 2019-2020 Variscite Ltd.
 */

#include <common.h>
#include <cpu_func.h>
#include <env.h>
#include <errno.h>
#include <init.h>
#include <linux/libfdt.h>
#include <fsl_esdhc_imx.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/snvs_security_sc.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <usb.h>

#include "../common/imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

extern int var_setup_mac(struct var_eeprom *eeprom);

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | \
			 (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
			 (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | \
			 (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

static iomux_cfg_t uart3_pads[] = {
	SC_P_SCU_GPIO0_00 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_SCU_GPIO0_01 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	int ret;

	/* Set UART0 clock root to 80 MHz - workaround to enable UART3 */
	ret = sc_pm_setup_uart(SC_R_UART_0, SC_80MHZ);
	if (ret)
		return ret;

	/* Set UART3 clock root to 80 MHz */
	ret = sc_pm_setup_uart(SC_R_UART_3, SC_80MHZ);
	if (ret)
		return ret;

	imx8_iomux_setup_multiple_pads(uart3_pads, ARRAY_SIZE(uart3_pads));

/* Dual bootloader feature will require CAAM access, but JR0 and JR1 will be
 * assigned to seco for imx8, use JR3 instead.
 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3_OUT, SC_PM_PW_MODE_ON);
#endif

	return 0;
}

int checkboard(void)
{
	print_bootinfo();

	return 0;
}

int board_init(void)
{
#ifdef CONFIG_SNVS_SEC_SC_AUTO
	int ret = snvs_security_sc_init();

	if (ret)
		return ret;
#endif

	return 0;
}

void board_quiesce_devices(void)
{
	const char *power_on_devices[] = {
		/* UART3 - Debug console */
		"PD_UART3_RX",
		"PD_UART3_TX",

		/* HIFI DSP boot */
		"audio_sai0",
		"audio_ocram",
	};

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	sc_pm_reboot(-1, SC_PM_RESET_TYPE_COLD);
	while(1);
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

#define SDRAM_SIZE_STR_LEN 5

int board_late_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];

	build_info();

	if (!var_eeprom_is_valid(ep))
		var_eeprom_read_header(ep);

#ifdef CONFIG_FEC_MXC
	var_setup_mac(ep);
#endif
	var_eeprom_print_prod_info(ep);

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d",
		(int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "VAR-SOM-MX8X");
	env_set("board_rev", "iMX8QXP");
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#if defined(CONFIG_DM_ETH) && defined(CONFIG_USB_ETHER)
	int ret = usb_ether_init();
	if (ret) {
		puts("USB ethernet gadget init failed\n");
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
        return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
