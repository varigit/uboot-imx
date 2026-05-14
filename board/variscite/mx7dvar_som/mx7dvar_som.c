// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright (C) 2016-2025 Variscite Ltd.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 */

#include <command.h>
#include <display_options.h>
#include <init.h>
#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/io.h>
#include <fsl_esdhc.h>
#include <linux/sizes.h>
#include <mmc.h>
#include <miiphy.h>
#include <power/pmic.h>
#include <power/pfuze3000_pmic.h>
#include <splash.h>
#ifndef CONFIG_DM_USB
#include <usb.h>
#include <usb/ehci-ci.h>
#endif
#if defined(CONFIG_SYS_I2C_MXC) && !defined(CONFIG_DM_I2C)
#include <asm/mach-imx/mxc_i2c.h>
#endif

#include <asm/mach-imx/video.h>

#include "mx7dvar_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#define SD_DEV 0
#define EMMC_DEV 2

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

static int env_check(char *var, char *val)
{
	char *read_val;

	if (!val)
		return 0;

	read_val = env_get(var);

	if (read_val && (strcmp(read_val, val) == 0))
		return 1;

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

static int mx7d_var_eeprom_get_ram_size(void)
{
	struct mx7d_var_eeprom *e = VAR_EEPROM_DATA;

	if (!mx7d_var_eeprom_is_valid(e))
		return -EINVAL;

	return (e->dram_size * SZ_128M);
}

int dram_init(void)
{
	int eeprom_ram_size = mx7d_var_eeprom_get_ram_size();

	if (eeprom_ram_size > 0)
		gd->ram_size = eeprom_ram_size;
	else
		gd->ram_size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE);

	return 0;
}

static const iomux_v3_cfg_t wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static const iomux_v3_cfg_t uart1_pads[] = {
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

#ifdef CONFIG_VIDEO
static const iomux_v3_cfg_t pwm_pads[] = {
	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX7D_PAD_GPIO1_IO02__GPIO1_IO2 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static int setup_lcd(void)
{
	int ret;
	struct gpio_desc desc;

	imx_iomux_v3_setup_multiple_pads(pwm_pads, ARRAY_SIZE(pwm_pads));

	ret = dm_gpio_lookup_name("GPIO1_2", &desc);
	if (ret) {
		printf("%s lookup GPIO1_2 failed ret = %d\n", __func__, ret);
		return -ENODEV;
	}

	ret = dm_gpio_request(&desc, "lcd_backlight");
	if (ret) {
		printf("%s request lcd_backlight failed ret = %d\n", __func__, ret);
		return -ENODEV;
	}

	dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	/* Set Brightness to high */
	dm_gpio_set_value(&desc, 1);

	return 0;
}

#ifdef CONFIG_SPLASH_SCREEN
static void splash_set_source(void)
{
	if (!env_check("splashsourceauto", "yes"))
		return;

#ifdef CONFIG_NAND_BOOT
	env_set("splashsource", "nand");
#else
	if (mmc_get_env_dev() == SD_DEV)
		env_set("splashsource", "sd");
	else if (mmc_get_env_dev() == EMMC_DEV)
		env_set("splashsource", "emmc");
#endif
}

int splash_screen_prepare(void)
{
	int ret = 0;
	char sd_devpart_str[5];
	char emmc_devpart_str[5];
	u32 dev_part;

	dev_part = env_get_ulong("mmcrootpart", 10, 0);

	sprintf(sd_devpart_str, "0:%d", dev_part);
	sprintf(emmc_devpart_str, "2:%d", dev_part);

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
	return ret;
}
#endif /* CONFIG_SPLASH_SCREEN */
#else
static inline int setup_lcd(void) { return 0; }
#endif /* CONFIG_VIDEO */

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

#define CMD_MMC_DEV_LEN 16
void board_late_mmc_env_init(void)
{
	char cmd[CMD_MMC_DEV_LEN];
	u32 dev_no = mmc_get_env_dev();

	if (!env_check("mmcautodetect", "yes"))
		return;

	env_set_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	env_set_ulong("mmcblk", mmc_map_to_kernel_blk(dev_no));

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}

static void check_emmc(void)
{
	struct mmc *mmc;
	int err;

	mmc = find_mmc_device(EMMC_DEV);
	err = !mmc;
	if (!err) {
		/* Silence mmc_init since SOMs can be with or without eMMC */
		int is_silent = (gd->flags & GD_FLG_SILENT);

		if (!is_silent)
			gd->flags |= GD_FLG_SILENT;
		err = mmc_init(mmc);
		if (!is_silent)
			gd->flags &= ~GD_FLG_SILENT;
	}

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (err)
		env_set("som_rev", "NAND");
	else
		env_set("som_rev", "EMMC");
#endif

	if (err) {
		puts("No eMMC\n");
		return;
	}

	puts("eMMC:  ");
	print_size(mmc->capacity, "\n");
}

#ifdef CONFIG_FEC_MXC
static int setup_fec(int fec_id)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	if (fec_id == 0) {
		/* Use 125M anatop REF_CLK1 for ENET1, clear gpr1[13], gpr1[17]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
				(IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK |
				 IOMUXC_GPR_GPR1_GPR_ENET1_CLK_DIR_MASK), 0);
	} else {
		/* Use 125M anatop REF_CLK2 for ENET2, clear gpr1[14], gpr1[18]*/
		clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
				(IOMUXC_GPR_GPR1_GPR_ENET2_TX_CLK_SEL_MASK |
				 IOMUXC_GPR_GPR1_GPR_ENET2_CLK_DIR_MASK), 0);
	}

	return set_clk_enet(ENET_125MHZ);
}

#define AR8033_PHY_ID	0x004dd074
#define ADIN1300_PHY_ID	0x0283bc30

int board_phy_config(struct phy_device *phydev)
{
	int bmcr;

	switch (phydev->phy_id) {
	case AR8033_PHY_ID:
		printf("AR8033 PHY detected at addr %d\n", phydev->addr);
#ifndef CONFIG_DM_ETH
		/* Enable RGMII Tx clock delay */
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);
#endif
		break;
	case ADIN1300_PHY_ID:
		printf("ADIN1300 PHY detected at addr %d\n", phydev->addr);
		bmcr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);
		if (bmcr < 0)
			return bmcr;

		if (bmcr & BMCR_PDOWN) {
			bmcr &= ~BMCR_PDOWN;
			phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, bmcr);
		}
		break;
	default:
		printf("%s: unknown phy_id 0x%x at addr %d\n", __func__,
		       phydev->phy_id, phydev->addr);
		break;
	}

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif /* CONFIG_FEC_MXC */

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_FEC_MXC
	setup_fec(CFG_FEC_ENET_DEV);
#endif

	return 0;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd1", MAKE_CFGVAL(0x10, 0x10, 0x00, 0x00)},
	{"emmc", MAKE_CFGVAL(0x10, 0x2a, 0x00, 0x00)},
	/* TODO: Nand */
	{NULL,   0},
};
#endif

#ifdef	CONFIG_DM_PMIC
int power_init_board(void)
{
	struct udevice *dev;
	int ret, dev_id, rev_id;

	ret = pmic_get("pfuze3000@8", &dev);
	if (ret == -ENODEV)
		return 0;
	if (ret != 0)
		return ret;

	dev_id = pmic_reg_read(dev, PFUZE3000_DEVICEID);
	rev_id = pmic_reg_read(dev, PFUZE3000_REVID);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", dev_id, rev_id);

	pmic_clrsetbits(dev, PFUZE3000_LDOGCTL, 0, 1);

	return 0;
}
#endif

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

	check_emmc();

	if (IS_ENABLED(CONFIG_MMC))
		board_late_mmc_env_init();

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int)(gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	setup_lcd();

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	board_codec_detect();

	return 0;
}

int checkboard(void)
{
	puts("Board: Variscite VAR-SOM-MX7\n");

	return 0;
}
