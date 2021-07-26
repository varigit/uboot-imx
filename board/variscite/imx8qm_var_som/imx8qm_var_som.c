/*
 * Copyright 2017-2018 NXP
 * Copyright 2019-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <fsl_esdhc.h>
#include <i2c.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <power-domain.h>

#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>

#include "../common/imx8_eeprom.h"
#include "imx8qm_var_som.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

extern int var_setup_mac(struct var_eeprom *eeprom);

static iomux_cfg_t uart0_pads[] = {
	SC_P_UART0_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART0_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart0_pads, ARRAY_SIZE(uart0_pads));
}

int board_early_init_f(void)
{
	int ret;

	/* Set UART0 clock root to 80 MHz */
	ret = sc_pm_setup_uart(SC_R_UART_0, SC_80MHZ);
	if (ret)
		return ret;

	setup_iomux_uart();

/* Dual bootloader feature will require CAAM access, but JR0 and JR1 will be
 * assigned to seco for imx8, use JR3 instead.
 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_DUAL_BOOTLOADER)
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3, SC_PM_PW_MODE_ON);
	sc_pm_set_resource_power_mode(-1, SC_R_CAAM_JR3_OUT, SC_PM_PW_MODE_ON);
#endif

	return 0;
}

int var_get_board_id(struct var_eeprom *ep)
{
	if (var_eeprom_is_valid(ep) && (ep->somrev & 0x40))
		return SPEAR_MX8;
	else
		return VAR_SOM_MX8;
}

#if defined(CONFIG_MULTI_DTB_FIT) && !defined(CONFIG_SPL_BUILD)
int board_fit_config_name_match(const char *name)
{
	int board_id;
	struct var_eeprom *ep = VAR_EEPROM_DATA;

	board_id = var_get_board_id(ep);
	if ((board_id == SPEAR_MX8) && !strcmp(name, "fsl-imx8qm-var-spear"))
		return 0;
	else if ((board_id == VAR_SOM_MX8) && !strcmp(name, "fsl-imx8qm-var-som"))
		return 0;
	else
		return -1;
}
#endif

int checkboard(void)
{
	print_bootinfo();

	return 0;
}

int board_init(void)
{
	return 0;
}

void board_quiesce_devices(void)
{
	const char *power_on_devices[] = {
		"dma_lpuart0",
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

int board_late_init(void)
{
	int board_id;
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	struct var_eeprom eeprom = {0};
	int ret;

	if (!var_eeprom_is_valid(ep)) {

		ret = var_eeprom_read_header(&eeprom);
		if (ret) {
			printf("%s EEPROM read failed.\n", __func__);
			return -1;
		}
		memcpy(ep, &eeprom, sizeof(*ep));
	}

#if IS_ENABLED(CONFIG_FEC_MXC)
	var_setup_mac(ep);
#endif

	var_eeprom_print_prod_info(ep);

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	board_id = var_get_board_id(ep);
	if (board_id == VAR_SOM_MX8)
		env_set("board_name", "VAR-SOM-MX8");
	else if (board_id == SPEAR_MX8)
		env_set("board_name", "SPEAR-MX8");
	env_set("board_rev", "iMX8QM");
#endif

	env_set("sec_boot", "no");
#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef IMX_LOAD_HDMI_FIMRWARE
	char command[256];
	char fw_folder[] = "firmware/hdp";
	char *hdp_file = env_get("hdp_file");
	char *hdprx_file = env_get("hdprx_file");

	u32 dev_no = mmc_get_env_dev();
	/*default dynamic partition*/
	u32 part_no = FIRMWARE;

	sprintf(command, "load mmc %x:%x 0x%x %s/%s", dev_no, part_no,
			 IMX_HDMI_FIRMWARE_LOAD_ADDR,
			 fw_folder, hdp_file);
	ret = run_command(command, 0);
	if (ret) {
		/*try vendor*/
		part_no = VENDOR;
		sprintf(command, "load mmc %x:%x 0x%x %s/%s", dev_no, part_no,
				 IMX_HDMI_FIRMWARE_LOAD_ADDR,
				 fw_folder, hdp_file);
		ret = run_command(command, 0);
	}

	sprintf(command, "hdp load 0x%x", IMX_HDMI_FIRMWARE_LOAD_ADDR);
	run_command(command, 0);

	sprintf(command, "load mmc %x:%x 0x%x %s/%s", dev_no, part_no,
			 IMX_HDMI_FIRMWARE_LOAD_ADDR + IMX_HDMITX_FIRMWARE_SIZE,
			 fw_folder, hdprx_file);
	run_command(command, 0);

	sprintf(command, "hdprx load 0x%x",
			IMX_HDMI_FIRMWARE_LOAD_ADDR + IMX_HDMITX_FIRMWARE_SIZE);
	run_command(command, 0);
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

