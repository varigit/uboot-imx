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
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <dm/root.h>
#include <fdtdec.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>
#include <cdns3-uboot.h>
#include <asm/arch/lpcg.h>

#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>

#include "../common/imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))
#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

extern int var_setup_mac(struct var_eeprom *eeprom);
extern int var_scu_eeprom_read_header(struct var_eeprom *e);

static iomux_cfg_t uart0_pads[] = {
	SC_P_UART0_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART0_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart0_pads, ARRAY_SIZE(uart0_pads));
}

enum {
	BOARD_UNKNOWN,
	BOARD_SPEAR_MX8,
	BOARD_VAR_SOM_MX8,
};

#if defined(CONFIG_DTB_RESELECT) && !defined(CONFIG_SPL_BUILD)
static void detect_board(void)
{
	int ret;
	struct udevice *dev;
	struct var_eeprom eeprom;

	/* Probe Message Unit (MU) - required to access SCU */
	uclass_find_first_device(UCLASS_MISC, &dev);
	for (; dev; uclass_find_next_device(&dev))
		if (device_probe(dev))
			continue;

	/* Read EEPROM */
	ret = var_scu_eeprom_read_header(&eeprom);
	if (ret) {
		debug("Board detection failed, assuming VAR-SOM-MX8\n");
		return;
	}

	/* Detect board type */
	if (eeprom.somrev & 0x40)
		gd->board_type = BOARD_SPEAR_MX8;
	else
		gd->board_type = BOARD_VAR_SOM_MX8;
}

int board_fit_config_name_match(const char *name)
{
	if ((gd->board_type == BOARD_UNKNOWN) &&
		!strcmp(name, "fsl-imx8qm-var-som"))
		return 0;
	if ((gd->board_type == BOARD_SPEAR_MX8) &&
		!strcmp(name, "fsl-imx8qm-var-spear"))
		return 0;
	else if ((gd->board_type == BOARD_VAR_SOM_MX8) &&
		!strcmp(name, "fsl-imx8qm-var-som"))
		return 0;
	else
		return -1;
}

int embedded_dtb_select(void)
{
	int rescan;

	detect_board();
	fdtdec_resetup(&rescan);

	return 0;
}
#endif

int board_early_init_f(void)
{
	int ret;

	/* When start u-boot in XEN VM, directly return */
	if (IS_ENABLED(CONFIG_XEN)) {
		writel(0xF53535F5, (void __iomem *)0x80000000);
		return 0;
	}

	/* Set UART0 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;

	/* Power up UART0 */
	ret = sc_pm_set_resource_power_mode(-1, SC_R_UART_0, SC_PM_PW_MODE_ON);
	if (ret)
		return ret;

	ret = sc_pm_set_clock_rate(-1, SC_R_UART_0, 2, &rate);
	if (ret)
		return ret;

	/* Enable UART0 clock root */
	ret = sc_pm_clock_enable(-1, SC_R_UART_0, 2, true, false);
	if (ret)
		return ret;

	lpcg_all_clock_on(LPUART_0_LPCG);

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

	if (IS_ENABLED(CONFIG_XEN)) {
		/* Clear magic number to let xen know uboot is over */
		writel(0x0, (void __iomem *)0x80000000);
		return;
	}

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	/* TODO */
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_late_init(void)
{
	int ret;
	struct var_eeprom eeprom;

	ret = var_eeprom_read_header(&eeprom);
	if (!ret) {
#if IS_ENABLED(CONFIG_FEC_MXC)
		var_setup_mac(&eeprom);
#endif
		var_eeprom_print_prod_info(&eeprom);
	}

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (gd->board_type == BOARD_VAR_SOM_MX8)
		env_set("board_name", "VAR-SOM-MX8");
	else if (gd->board_type == BOARD_SPEAR_MX8)
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

#ifdef CONFIG_IMX_LOAD_HDMI_FIMRWARE
	char command[256];
	char fw_folder[] = "firmware/hdp";
	char *hdp_file = env_get("hdp_file");
	char *hdprx_file = env_get("hdprx_file");
	u32 dev_no = mmc_get_env_dev();
	u32 part_no = 10;

	sprintf(command, "load mmc %x:%x 0x%x %s/%s", dev_no, part_no,
			 IMX_HDMI_FIRMWARE_LOAD_ADDR,
			 fw_folder, hdp_file);
	run_command(command, 0);

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

#if defined(CONFIG_VIDEO_IMXDPUV1)

#define BACKLIGHT_GPIO IMX_GPIO_NR(1, 4)

static iomux_cfg_t backlight_pads[] = {
	SC_P_LVDS0_GPIO00 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void enable_backlight(void)
{
	imx8_iomux_setup_multiple_pads(backlight_pads,
			ARRAY_SIZE(backlight_pads));

	/* Set brightness to high */
	gpio_request(BACKLIGHT_GPIO, "backlight");
	gpio_direction_output(BACKLIGHT_GPIO, 1);
}

static void enable_lvds(struct display_info_t const *dev)
{
	display_controller_setup((PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_soc_setup(dev->bus, (PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_configure(dev->bus);
	enable_backlight();
}

struct display_info_t const displays[] = {{
	.bus	= 0, /* LVDS0 */
	.addr	= 0, /* Unused */
	.pixfmt	= IMXDPUV1_PIX_FMT_BGRA32,
	.detect	= NULL,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "VAR-WVGA-LCD",
		.xres           = 800,
		.yres           = 480,
		.pixclock       = KHZ2PICOS(35714),
		.left_margin    = 40,
		.right_margin   = 40,
		.upper_margin   = 29,
		.lower_margin   = 13,
		.hsync_len      = 48,
		.vsync_len      = 3,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);

#endif /* CONFIG_VIDEO_IMXDPUV1 */
