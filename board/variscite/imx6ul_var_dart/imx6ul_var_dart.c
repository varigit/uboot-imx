// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 * Copyright (C) 2019 Parthiban Nallathambi <parthitce@gmail.com>
 * Copyright (C) 2021 Marc Ferland, Amotus Solutions Inc., <ferlandm@amotus.ca>
 */
#include <command.h>
#include <dm.h>
#include <fsl_esdhc_imx.h>
#include <i2c.h>
#include <malloc.h>
#include <miiphy.h>
#include <splash.h>
#include <asm/arch/clock.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/iomux-v3.h>

#include "imx6ul_var_dart-common.h"
#include "mx6var_eeprom_v2.h"

#define SDRAM_SIZE_STR_LEN 5

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();
	return 0;
}

#ifdef CONFIG_FEC_MXC
static int setup_fec(int fec_id)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int ret;

	if (fec_id == 0) {
		/*
		 * Use 50M anatop loopback REF_CLK1 for ENET1,
		 * clear gpr1[13], set gpr1[17].
		 */
		clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
				IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);
	} else {
		/*
		 * Use 50M anatop loopback REF_CLK2 for ENET2,
		 * clear gpr1[14], set gpr1[18].
		 */
		clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC2_MASK,
				IOMUX_GPR1_FEC2_CLOCK_MUX1_SEL_MASK);
	}

	ret = enable_fec_anatop_clock(fec_id, ENET_50MHZ);
	if (ret)
		return ret;

	enable_enet_clk(1);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	/*
	 * Defaults + Enable status LEDs (LED1: Activity, LED0: Link) & select
	 * 50 MHz RMII clock mode.
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8190);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif /* CONFIG_FEC_MXC */

#if IS_ENABLED(CONFIG_VIDEO)

static const iomux_v3_cfg_t pwm_pads[][2] = {
	/* Use GPIO for Brightness adjustment, duty cycle = period */
	{
		/* DART-6UL */
		MX6_PAD_LCD_DATA00__GPIO3_IO05 | MUX_PAD_CTRL(NO_PAD_CTRL),
	},
	{
		/* VAR-SOM-6UL */
		MX6_PAD_GPIO1_IO05__GPIO1_IO05 | MUX_PAD_CTRL(NO_PAD_CTRL),
	},
};

#define DART_6UL_BACKLIGHT_PWM		IMX_GPIO_NR(3, 5)
#define VAR_SOM_6UL_BACKLIGHT_PWM	IMX_GPIO_NR(1, 5)

static int backlight_gpio[] = {
	DART_6UL_BACKLIGHT_PWM,
	VAR_SOM_6UL_BACKLIGHT_PWM
};

static void setup_lcd(void)
{
	if (!is_mx6ulz()) {
		int board = get_board_indx();

		enable_lcdif_clock(LCDIF1_BASE_ADDR, 1);

		imx_iomux_v3_setup_multiple_pads(pwm_pads[board], ARRAY_SIZE(pwm_pads));

		/* Set Brightness to high */
		gpio_request(backlight_gpio[board], "backlight");
		gpio_direction_output(backlight_gpio[board], 1);
	}
}

/*
 * Turn off backlight before OS handover
 */
void board_preboot_os(void)
{
	if (!is_mx6ulz()) {
		int board = get_board_indx();
		gpio_direction_output(backlight_gpio[board], 0);
	}
}

#if IS_ENABLED(CONFIG_SPLASH_SCREEN)
static void splash_set_source(void)
{
	if (env_get_yesno("splashsourceauto") != 1)
		return;

#if IS_ENABLED(CONFIG_NAND_BOOT)
	env_set("splashsource", "nand");
#else
	if (mmc_get_env_dev() == 0)
		env_set("splashsource", "sd");
	else if (mmc_get_env_dev() == 1)
		env_set("splashsource", "emmc");
#endif
}

int splash_screen_prepare(void)
{
	int ret = -ENOSYS;

	if (!is_mx6ulz()) {
		char sd_devpart_str[5];
		char emmc_devpart_str[5];
		u32 dev_part;

		dev_part = env_get_ulong("mmcrootpart", 10, 0);

		sprintf(sd_devpart_str, "0:%d", dev_part);
		sprintf(emmc_devpart_str, "1:%d", dev_part);

		struct splash_location var_splash_locations[] = {
			{
				.name = "sd",
				.storage = SPLASH_STORAGE_MMC,
				.flags = SPLASH_STORAGE_FS,
				.devpart = sd_devpart_str,
			},
			{
				.name = "emmc",
				.storage = SPLASH_STORAGE_MMC,
				.flags = SPLASH_STORAGE_FS,
				.devpart = emmc_devpart_str,
			},
			{
				.name = "nand",
				.storage = SPLASH_STORAGE_NAND,
				.flags = SPLASH_STORAGE_FS,
				.mtdpart = "rootfs",
				.ubivol = "ubi0:rootfs",
			},
		};

		splash_set_source();

		ret = splash_source_load(var_splash_locations,
					 ARRAY_SIZE(var_splash_locations));
	}

	return ret;
}
#endif /* CONFIG_SPLASH_SCREEN */
#else
static inline int setup_lcd(void) { return 0; }
#endif

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	if (IS_ENABLED(CONFIG_FEC_MXC) && !is_mx6ulz())
		setup_fec(CFG_FEC_ENET_DEV);

	return 0;
}

#if IS_ENABLED(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG)

#define CODEC_I2C_BUS	1
#define CODEC_I2C_ADDR	0x1a
#define CODEC_CHIP_ID	0
#define CODEC_WM8904	0x0489 /* bytes swapped */

static void board_codec_detect(void)
{
	struct udevice *bus, *codec_dev;
	u8 is_silent;
	u16 id;
	int ret;

	/* Locate the I2C bus */
	ret = uclass_get_device_by_seq(UCLASS_I2C, CODEC_I2C_BUS, &bus);
	if (ret) {
		printf("Couldn't find I2C bus %d\n", CODEC_I2C_BUS);
		env_set("codec", "none");
		return;
	}

	/* Probe the codec device on the bus */
	ret = dm_i2c_probe(bus, CODEC_I2C_ADDR, 0, &codec_dev);
	if (ret) {
		printf("Couldn't find audio codec device at 0x%x\n", CODEC_I2C_ADDR);
		env_set("codec", "none");
		return;
	}

	/*
	 * Silence the ID read operation, as in case of the
	 * codec wm8731, being a write-only device, this will
	 * end up in printing errors.
	 */
	is_silent = (gd->flags & GD_FLG_SILENT);
	if (!is_silent)
		gd->flags |= GD_FLG_SILENT;

	/* Read codec ID */
	ret = dm_i2c_read(codec_dev, CODEC_CHIP_ID, (u8 *)&id, sizeof(id));
	if (ret) {
		/* Recover by re-probing */
		printf("Error reading codec ID, assuming wm8731\n");
		ret = dm_i2c_probe(bus, CODEC_I2C_ADDR, 0, &codec_dev);
		env_set("codec", "wm8731");
	} else if (id == CODEC_WM8904) {
		env_set("codec", "wm8904");
	} else {
		env_set("codec", "unknown");
	}

	if (!is_silent)
		gd->flags &= ~GD_FLG_SILENT;

	printf("Codec: %s\n", env_get("codec"));
}
#endif

int checkboard(void)
{
	struct var_eeprom *e = VAR_EEPROM_DATA;

	if (e->magic != DART6UL_INFO_MAGIC) {
		printf("Board: Invalid eeprom magic: 0x%08x, expected 0x%08x\n",
		       e->magic, DART6UL_INFO_MAGIC);
		return 0;
	}

	var_eeprom_print_prod_infos(e);

	return 0;
}

#define SDRAM_SIZE_STR_LEN 5

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

#define CMD_MMC_DEV_LEN 16
void board_late_mmc_init(void)
{
	char cmd[CMD_MMC_DEV_LEN];
	u32 dev_no = mmc_get_env_dev();

	if (env_get_yesno("mmcautodetect") != 1)
		return;

	env_set_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	env_set_ulong("mmcblk", mmc_map_to_kernel_blk(dev_no));

	snprintf(cmd, CMD_MMC_DEV_LEN, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}

static void setup_env_vars(struct var_eeprom *e)
{
	char sdram_size_str[SDRAM_SIZE_STR_LEN];

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int)(gd->ram_size / 1024 / 1024));

	env_set("sdram_size", sdram_size_str);
	env_set("wifi", DART6UL_INFO_WIFI_GET(e->som_info) ? "yes" : "no");
	env_set("som_rev", som_info_rev_to_str(e->som_info));
	env_set("som_storage", som_info_storage_to_str(e->som_info));
}

int board_late_init(void)
{
	struct var_eeprom *e = VAR_EEPROM_DATA;

	setup_lcd();

	if (IS_ENABLED(CONFIG_MMC))
		board_late_mmc_init();

	if (IS_ENABLED(CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG))
		env_set("board_name", is_dart_6ul() ? "DART-6UL" : "VAR-SOM-6UL");

	env_set("soc_type", is_cpu_type(MXC_CPU_MX6ULL) ? "imx6ull" :
		is_mx6ulz() ? "imx6ulz" : "imx6ul");

	switch (get_boot_device()) {
	case SD1_BOOT:
	case MMC1_BOOT:
		env_set("boot_dev", "sd");
		break;
	case SD2_BOOT:
	case MMC2_BOOT:
		env_set("boot_dev", "emmc");
		break;
	case NAND_BOOT:
		env_set("boot_dev", "nand");
		break;
	default:
		env_set("boot_dev", "unknown");
		break;
	}

	setup_env_vars(e);
	board_codec_detect();

	return 0;
}

#if IS_ENABLED(CONFIG_LDO_BYPASS_CHECK)
void ldo_mode_set(int ldo_bypass)
{
}
#endif /* CONFIG_LDO_BYPASS_CHECK */
