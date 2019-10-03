/*
 * Copyright 2017-2018 NXP
 * Copyright 2019 Variscite Ltd.
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
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>
#include <cdns3-uboot.h>
#include <miiphy.h>
#include <splash.h>

#include "../common/imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))
#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

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
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* Power up UART0, this is very early while power domain is not working */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_0, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set UART0 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_0, 2, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable UART0 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_0, 2, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_FEC_MXC
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

static int setup_mac(struct var_eeprom *eeprom)
{
	int ret;
	uint64_t addr;
	unsigned char enetaddr[6];
	unsigned char enet1addr[6];

	ret = eth_env_get_enetaddr("ethaddr", enetaddr);
	if (ret)
		return 0;

	ret = var_eeprom_get_mac(eeprom, enetaddr);
	if (ret)
		return ret;

	if (!is_valid_ethaddr(enetaddr))
		return -1;

	addr = mac2int(enetaddr);
	int2mac(addr + 1, enet1addr);

	eth_env_set_enetaddr("ethaddr", enetaddr);
	eth_env_set_enetaddr("eth1addr", enet1addr);

	return 0;
}
#endif /* CONFIG_FEC_MXC */

enum {
	SPEAR_MX8   =  1,
	VAR_SOM_MX8 =  2,
};

static int get_board_id(void)
{
	struct var_eeprom eeprom;
	static int board_id = -1;

	if (board_id == -1) {
		if (!var_scu_eeprom_read_header(&eeprom) &&
		    var_eeprom_is_valid(&eeprom) && (eeprom.somrev & 0x40))
			board_id = SPEAR_MX8;
		else
			board_id = VAR_SOM_MX8;
	}

	return board_id;
}

#ifdef CONFIG_MULTI_DTB_FIT
int board_fit_config_name_match(const char *name)
{
	int board_id = get_board_id();

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
	int board_id = get_board_id();

	if (board_id == SPEAR_MX8)
		puts("Board: SPEAR-MX8\n");
	else if (board_id == VAR_SOM_MX8)
		puts("Board: VAR-SOM-MX8\n");
	else
		puts("Board: Unknown\n");

	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH) {
		printf("\nSCI error! Invalid handle\n");
	}

	return 0;
}

#ifdef CONFIG_USB_CDNS3_GADGET
static struct cdns3_device cdns3_device_data = {
	.none_core_base = 0x5B110000,
	.xhci_base = 0x5B130000,
	.dev_base = 0x5B140000,
	.phy_base = 0x5B160000,
	.otg_base = 0x5B120000,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 1,
};

int usb_gadget_handle_interrupts(void)
{
	cdns3_uboot_handle_interrupt(1);
	return 0;
}
#endif

int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_CDNS3_GADGET
		} else {
#ifdef CONFIG_SPL_BUILD
			sc_ipc_t ipcHndl = 0;

			ipcHndl = gd->arch.ipc_channel_handle;

			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_2, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				printf("conn_usb2 Power up failed! (error = %d)\n", ret);

			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_2_PHY, SC_PM_PW_MODE_ON);
			if (ret != SC_ERR_NONE)
				printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
#else
			struct power_domain pd;
			int ret;

			/* Power on usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2 Power up failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_on(&pd);
				if (ret)
					printf("conn_usb2_phy Power up failed! (error = %d)\n", ret);
			}
#endif

			ret = cdns3_uboot_init(&cdns3_device_data);
			printf("%d cdns3_uboot_initmode %d\n", index, ret);
#endif
		}
	}
	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	if (index == 1) {
		if (init == USB_INIT_HOST) {
#ifdef CONFIG_USB_CDNS3_GADGET
		} else {
			cdns3_uboot_exit(1);

#ifdef CONFIG_SPL_BUILD
			sc_ipc_t ipcHndl = 0;

			ipcHndl = gd->arch.ipc_channel_handle;

			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_2, SC_PM_PW_MODE_OFF);
			if (ret != SC_ERR_NONE)
				printf("conn_usb2 Power down failed! (error = %d)\n", ret);

			ret = sc_pm_set_resource_power_mode(ipcHndl, SC_R_USB_2_PHY, SC_PM_PW_MODE_OFF);
			if (ret != SC_ERR_NONE)
				printf("conn_usb2_phy Power down failed! (error = %d)\n", ret);
#else
			struct power_domain pd;
			int ret;

			/* Power off usb */
			if (!power_domain_lookup_name("conn_usb2", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2 Power down failed! (error = %d)\n", ret);
			}

			if (!power_domain_lookup_name("conn_usb2_phy", &pd)) {
				ret = power_domain_off(&pd);
				if (ret)
					printf("conn_usb2_phy Power down failed! (error = %d)\n", ret);
			}
#endif
#endif
		}
	}
	return ret;
}

int board_init(void)
{
	return 0;
}

void board_quiesce_devices()
{
	const char *power_on_devices[] = {
		"dma_lpuart0",
	};

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
	puts("SCI reboot request");
	sc_pm_reboot(SC_IPC_CH, SC_PM_RESET_TYPE_COLD);
	while (1)
		putc('.');
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int board_late_init(void)
{
	struct var_eeprom eeprom;
	int board_id = get_board_id();

	var_eeprom_read_header(&eeprom);

#ifdef CONFIG_FEC_MXC
	setup_mac(&eeprom);
#endif
	var_eeprom_print_prod_info(&eeprom);

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
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

#ifdef CONFIG_SPLASH_SCREEN

static int check_env(char *var, char *val)
{
	char *env_val = env_get(var);

	if ((env_val != NULL) &&
		(strcmp(env_val, val) == 0)) {
		return 1;
	}

	return 0;
}

static void splash_set_source(void)
{
	if (!check_env("splashsourceauto", "yes"))
		return;

	if (mmc_get_env_dev() == 0)
		env_set("splashsource", "emmc");
	else if (mmc_get_env_dev() == 1)
		env_set("splashsource", "sd");
}

int splash_screen_prepare(void)
{
	char sd_devpart[5];
	char emmc_devpart[5];
	u32 sd_part, emmc_part;

	sd_part = emmc_part = env_get_ulong("mmcpart", 10, 0);

	sprintf(sd_devpart, "1:%d", sd_part);
	sprintf(emmc_devpart, "0:%d", emmc_part);

	struct splash_location splash_locations[] = {
		{
			.name = "sd",
			.storage = SPLASH_STORAGE_MMC,
			.flags = SPLASH_STORAGE_FS,
			.devpart = sd_devpart,
		},
		{
			.name = "emmc",
			.storage = SPLASH_STORAGE_MMC,
			.flags = SPLASH_STORAGE_FS,
			.devpart = emmc_devpart,
		}
	};

	splash_set_source();

	return splash_source_load(splash_locations,
			ARRAY_SIZE(splash_locations));

}
#endif /* CONFIG_SPLASH_SCREEN */
