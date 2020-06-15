/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright (C) 2016-2019 Variscite Ltd.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <linux/sizes.h>
#include <mmc.h>
#include <miiphy.h>
#include <netdev.h>
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

#ifdef CONFIG_VIDEO_MXS
#include <asm/mach-imx/video.h>
#include "../drivers/video/mxcfb.h"
#endif

#ifdef CONFIG_FSL_FASTBOOT
#include <fsl_fastboot.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_SYS_I2C
#include "mx7dvar_eeprom.h"
#endif


DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | \
	PAD_CTL_PUS_PU100KOHM | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_49OHM)
#define ENET_PAD_CTRL_MII  (PAD_CTL_DSE_3P3V_32OHM)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PUS_PU100KOHM | PAD_CTL_DSE_3P3V_49OHM)

#define I2C_PAD_CTRL    (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_PU100KOHM | \
	PAD_CTL_DSE_3P3V_98OHM)

#define BUTTON_PAD_CTRL    (PAD_CTL_PUS_PU5KOHM | PAD_CTL_DSE_3P3V_98OHM)

#define NAND_PAD_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_SRE_SLOW | PAD_CTL_HYS)

#define NAND_PAD_READY0_CTRL (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU5KOHM)

int board_mmc_get_env_dev(int devno)
{
	if (2 == devno)
		devno--;

	return devno;
}

static int env_check(char *var, char *val)
{
	char *read_val;
	if (var == NULL || val == NULL)
		return 0;

	read_val = env_get(var);

	if ((read_val != NULL) &&
			(strcmp(read_val, val) == 0)) {
		return 1;
	}

	return 0;
}

#if defined(CONFIG_SYS_I2C_MXC) && !defined(CONFIG_DM_I2C)
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C1_SCL__GPIO4_IO8 | PC,
		.gp = IMX_GPIO_NR(4, 8),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C1_SDA__GPIO4_IO9 | PC,
		.gp = IMX_GPIO_NR(4, 9),
	},
};

/* I2C3 */
struct i2c_pads_info i2c_pad_info3 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C3_SCL__I2C3_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C3_SCL__GPIO4_IO12 | PC,
		.gp = IMX_GPIO_NR(4, 12),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C3_SDA__I2C3_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C3_SDA__GPIO4_IO13 | PC,
		.gp = IMX_GPIO_NR(4, 13),
	},
};
#endif

#ifdef CONFIG_SYS_I2C
static int var_eeprom_get_ram_size(void)
{
	u16 read_eeprom_magic;
	u8  read_ram_size;

	i2c_set_bus_num(EEPROM_I2C_BUS);
	if (i2c_probe(EEPROM_I2C_ADDR)) {
		eeprom_debug("\nError: Couldn't find EEPROM device\n");
		return -1;
	}

	if ((i2c_read(EEPROM_I2C_ADDR,
			offsetof(struct var_eeprom_cfg, eeprom_magic),
			1,
			(u8 *) &read_eeprom_magic,
			sizeof(read_eeprom_magic))) ||
	   (i2c_read(EEPROM_I2C_ADDR,
			offsetof(struct var_eeprom_cfg, dram_size),
			1,
			(u8 *) &read_ram_size,
			sizeof(read_ram_size))))
	{
		eeprom_debug("\nError reading data from EEPROM\n");
		return -1;
	}

	if (EEPROM_MAGIC != read_eeprom_magic) {
		eeprom_debug("\nError: Data on EEPROM is invalid\n");
		return -1;
	}

	return (read_ram_size * SZ_128M);
}
#endif

int dram_init(void)
{
#ifdef CONFIG_SYS_I2C
	int eeprom_ram_size = var_eeprom_get_ram_size();

	if (eeprom_ram_size > 0)
		gd->ram_size = eeprom_ram_size;
	else
#endif
		gd->ram_size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE);

	return 0;
}

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const uart1_pads[] = {
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

#ifdef CONFIG_NAND_MXS
static iomux_v3_cfg_t const gpmi_pads[] = {
	MX7D_PAD_SD3_DATA0__NAND_DATA00 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__NAND_DATA01 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__NAND_DATA02 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__NAND_DATA03 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__NAND_DATA04 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__NAND_DATA05 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__NAND_DATA06 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__NAND_DATA07 | MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CLK__NAND_CLE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_CMD__NAND_ALE	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_STROBE__NAND_RE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__NAND_WE_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_MCLK__NAND_WP_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_RX_DATA__NAND_CE1_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_BCLK__NAND_CE0_B	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_SYNC__NAND_DQS	| MUX_PAD_CTRL(NAND_PAD_CTRL),
	MX7D_PAD_SAI1_TX_DATA__NAND_READY_B	| MUX_PAD_CTRL(NAND_PAD_READY0_CTRL),
};

static void setup_gpmi_nand(void)
{
	imx_iomux_v3_setup_multiple_pads(gpmi_pads, ARRAY_SIZE(gpmi_pads));

	/* NAND_USDHC_BUS_CLK is set in rom */

	set_clk_nand();
}
#endif

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lcd_pads[] = {
	MX7D_PAD_EPDC_DATA00__LCD_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA01__LCD_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA03__LCD_HSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA02__LCD_VSYNC | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA02__LCD_DATA2 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA03__LCD_DATA3 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA04__LCD_DATA4 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA05__LCD_DATA5 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA06__LCD_DATA6 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA07__LCD_DATA7 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA10__LCD_DATA10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA11__LCD_DATA11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA12__LCD_DATA12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA13__LCD_DATA13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA14__LCD_DATA14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_EPDC_DATA15__LCD_DATA15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA18__LCD_DATA18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA19__LCD_DATA19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA20__LCD_DATA20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA21__LCD_DATA21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA22__LCD_DATA22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX7D_PAD_LCD_DATA23__LCD_DATA23 | MUX_PAD_CTRL(LCD_PAD_CTRL),
};

static iomux_v3_cfg_t const pwm_pads[] = {
	/* Use GPIO for Brightness adjustment, duty cycle = period */
	MX7D_PAD_GPIO1_IO02__GPIO1_IO2 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

void do_enable_parallel_lcd(struct display_info_t const *dev)
{
	imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

	imx_iomux_v3_setup_multiple_pads(pwm_pads, ARRAY_SIZE(pwm_pads));

	/* Set Brightness to high */
	gpio_request(IMX_GPIO_NR(1, 2), "lcd_backlight");
	gpio_direction_output(IMX_GPIO_NR(1, 2) , 1);
}

#define MHZ2PS(f)	(1000000/(f))

struct display_info_t const displays[] = {{
	.bus = ELCDIF1_IPS_BASE_ADDR,
	.addr = 0,
	.pixfmt = 24,
	.detect = NULL,
	.enable = do_enable_parallel_lcd,
	.mode = {
		.name		= "VAR-WVGA-LCD",
		.xres		= 800,
		.yres		= 480,
		.pixclock	= MHZ2PS(30),
		.left_margin	= 40,
		.right_margin	= 40,
		.upper_margin	= 29,
		.lower_margin	= 13,
		.hsync_len	= 48,
		.vsync_len	= 3,
		.sync		= FB_SYNC_CLK_LAT_FALL,
		.vmode		= FB_VMODE_NONINTERLACED
	}
}};
size_t display_count = ARRAY_SIZE(displays);
#endif /* CONFIG_VIDEO_MXS */

#ifdef CONFIG_SPLASH_SCREEN
static void set_splashsource_to_boot_rootfs(void)
{
	if (!env_check("splashsourceauto", "yes"))
		return;

#ifdef CONFIG_NAND_BOOT
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
	int ret=0;
	char sd_devpart_str[5];
	char emmc_devpart_str[5];
	u32 sd_part, emmc_part;

	sd_part = emmc_part = env_get_ulong("mmcrootpart", 10, 0);

	sprintf(sd_devpart_str, "0:%d", sd_part);
	sprintf(emmc_devpart_str, "1:%d", emmc_part);

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

	set_splashsource_to_boot_rootfs();

	ret = splash_source_load(var_splash_locations,
			ARRAY_SIZE(var_splash_locations));

	return ret;
}
#endif /* CONFIG_SPLASH_SCREEN */

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

#ifndef CONFIG_DM_MMC

static iomux_v3_cfg_t const usdhc1_pads[] = {
	MX7D_PAD_SD1_CLK__SD1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_CMD__SD1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD1_CD_B__GPIO5_IO0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_RESET_B__GPIO5_IO2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_emmc_pads[] = {
	MX7D_PAD_SD3_CLK__SD3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_CMD__SD3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__SD3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__SD3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__SD3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__SD3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD3_RESET_B__GPIO6_IO11 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

#define USDHC1_CD_GPIO	IMX_GPIO_NR(5, 0)
#define USDHC1_PWR_GPIO	IMX_GPIO_NR(5, 2)
#define USDHC3_PWR_GPIO IMX_GPIO_NR(6, 11)

static struct fsl_esdhc_cfg usdhc_cfg[2];

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC1_BASE_ADDR:
		ret = !gpio_get_value(USDHC1_CD_GPIO);
		break;
	case USDHC3_BASE_ADDR:
		ret = 1; /* Assume uSDHC3 emmc is always present */
		break;
	}

	return ret;
}

int board_mmc_init(bd_t *bis)
{
#ifndef CONFIG_SPL_BUILD
	int i, ret;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC1 (SD)
	 * mmc1                    USDHC3 (eMMC)
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
				usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
			gpio_request(USDHC1_CD_GPIO, "usdhc1_cd");
			gpio_direction_input(USDHC1_CD_GPIO);
			gpio_request(USDHC1_PWR_GPIO, "usdhc1_pwr");
			gpio_direction_output(USDHC1_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC1_PWR_GPIO, 1);
			usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			usdhc_cfg[0].max_bus_width = 4;
			break;
		case 1:
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_emmc_pads, ARRAY_SIZE(usdhc3_emmc_pads));
			gpio_request(USDHC3_PWR_GPIO, "usdhc3_pwr");
			gpio_direction_output(USDHC3_PWR_GPIO, 0);
			udelay(500);
			gpio_direction_output(USDHC3_PWR_GPIO, 1);
			usdhc_cfg[1].esdhc_base = USDHC3_BASE_ADDR;
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return 0;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
#else
        /*
         * Possible MMC boot devices:
         * USDHC1 (SD)
         * USDHC3 (eMMC)
         */
        puts("MMC Boot Device: ");
        switch (get_boot_device()) {
	case SD1_BOOT:
	case MMC1_BOOT:
		puts("mmc0 (SD)");
		imx_iomux_v3_setup_multiple_pads(
				usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
		gpio_request(USDHC1_CD_GPIO, "usdhc1_cd");
		gpio_direction_input(USDHC1_CD_GPIO);
		gpio_request(USDHC1_PWR_GPIO, "usdhc1_pwr");
		gpio_direction_output(USDHC1_PWR_GPIO, 0);
		udelay(500);
		gpio_direction_output(USDHC1_PWR_GPIO, 1);
		usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
		usdhc_cfg[0].max_bus_width = 4;
		break;
	case SD3_BOOT:
	case MMC3_BOOT:
		puts("mmc1 (eMMC)");
		imx_iomux_v3_setup_multiple_pads(
				usdhc3_emmc_pads, ARRAY_SIZE(usdhc3_emmc_pads));
		gpio_request(USDHC3_PWR_GPIO, "usdhc3_pwr");
		gpio_direction_output(USDHC3_PWR_GPIO, 0);
		udelay(500);
		gpio_direction_output(USDHC3_PWR_GPIO, 1);
		usdhc_cfg[0].esdhc_base = USDHC3_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
		break;
        default:
                break;
        }
        puts("\n");

        return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
#endif
}

#endif /* CONFIG_DM_MMC */

int mmc_map_to_kernel_blk(int dev_no)
{
	if (dev_no == 1)
		dev_no++;

	return dev_no;
}

void board_late_mmc_init(void)
{
	char cmd[32];
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

	mmc = find_mmc_device(1);
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
#ifndef CONFIG_DM_ETH
static iomux_v3_cfg_t const fec1_pads[] = {
	MX7D_PAD_SD2_RESET_B__GPIO5_IO11 | MUX_PAD_CTRL(NO_PAD_CTRL), /* phy1 reset */
	MX7D_PAD_ENET1_RGMII_RX_CTL__ENET1_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD0__ENET1_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD1__ENET1_RGMII_RD1 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD2__ENET1_RGMII_RD2 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RD3__ENET1_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_RXC__ENET1_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TX_CTL__ENET1_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD0__ENET1_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD1__ENET1_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD2__ENET1_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TD3__ENET1_RGMII_TD3 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_ENET1_RGMII_TXC__ENET1_RGMII_TXC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_SD2_CD_B__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL_MII),
	MX7D_PAD_SD2_WP__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL_MII),
};

static iomux_v3_cfg_t const fec2_pads[] = {
	MX7D_PAD_UART2_TX_DATA__GPIO4_IO3 | MUX_PAD_CTRL(NO_PAD_CTRL), /* phy2 reset */
	MX7D_PAD_EPDC_SDCE0__ENET2_RGMII_RX_CTL | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCLK__ENET2_RGMII_RD0 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDLE__ENET2_RGMII_RD1  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDOE__ENET2_RGMII_RD2  | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDSHR__ENET2_RGMII_RD3 | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE1__ENET2_RGMII_RXC | MUX_PAD_CTRL(ENET_RX_PAD_CTRL),
	MX7D_PAD_EPDC_GDRL__ENET2_RGMII_TX_CTL | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE2__ENET2_RGMII_TD0 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_SDCE3__ENET2_RGMII_TD1 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDCLK__ENET2_RGMII_TD2 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDOE__ENET2_RGMII_TD3  | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_EPDC_GDSP__ENET2_RGMII_TXC  | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX7D_PAD_SD2_CD_B__ENET1_MDIO | MUX_PAD_CTRL(ENET_PAD_CTRL_MII),
	MX7D_PAD_SD2_WP__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL_MII),
};

static void setup_iomux_fec(void)
{
	if (0 == CONFIG_FEC_ENET_DEV)
		imx_iomux_v3_setup_multiple_pads(fec1_pads, ARRAY_SIZE(fec1_pads));
	else
		imx_iomux_v3_setup_multiple_pads(fec2_pads, ARRAY_SIZE(fec2_pads));
}

int board_eth_init(bd_t *bis)
{
	int ret;

	setup_iomux_fec();

	ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
		CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	if (ret)
		printf("FEC%d MXC: %s:failed\n", CONFIG_FEC_ENET_DEV, __func__);

#if defined(CONFIG_CI_UDC) && defined(CONFIG_USB_ETHER)
	/* USB Ethernet Gadget */
	usb_eth_initialize(bis);
#endif

	return ret;
}
#endif /* CONFIG_DM_ETH */

static int setup_fec(int fec_id)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;
	int ret, phy_rst_gpio;

	if (0 == fec_id) {
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

	ret = set_clk_enet(ENET_125MHZ);
	if (ret) {
		printf("FEC%d %s:failed\n", fec_id, __func__);
		return ret;
	}

	phy_rst_gpio = (0 == fec_id) ?
		IMX_GPIO_NR(5, 11) :
		IMX_GPIO_NR(4, 3);
	gpio_request(phy_rst_gpio, "eth_phy_rst");
	gpio_direction_output(phy_rst_gpio, 0);
	mdelay(10);
	gpio_direction_output(phy_rst_gpio, 1);

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
#ifndef CONFIG_DM_ETH
	/* Enable RGMII Tx clock delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);
#endif

	if (phydev->drv->config)
		phydev->drv->config(phydev);
	return 0;
}
#endif /* CONFIG_FEC_MXC */

int board_early_init_f(void)
{
	setup_iomux_uart();

#if defined(CONFIG_SYS_I2C_MXC) && !defined(CONFIG_DM_I2C)
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info3);
#endif

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

#ifdef CONFIG_FEC_MXC
	setup_fec(CONFIG_FEC_ENET_DEV);
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
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

	ret = pmic_get("pfuze3000", &dev);
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
#ifdef CONFIG_POWER
#define I2C_PMIC	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;
	unsigned int reg, rev_id;

	ret = power_pfuze3000_init(I2C_PMIC);
	if (ret)
		return ret;

	p = pmic_get("PFUZE3000");
	ret = pmic_probe(p);
	if (ret)
		return ret;

	pmic_reg_read(p, PFUZE3000_DEVICEID, &reg);
	pmic_reg_read(p, PFUZE3000_REVID, &rev_id);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", reg, rev_id);

	/* disable Low Power Mode during standby mode */
	pmic_reg_read(p, PFUZE3000_LDOGCTL, &reg);
	reg |= 0x1;
	pmic_reg_write(p, PFUZE3000_LDOGCTL, reg);

	/* SW1A/1B mode set to APS/APS */
	reg = 0x8;
	pmic_reg_write(p, PFUZE3000_SW1AMODE, reg);
	pmic_reg_write(p, PFUZE3000_SW1BMODE, reg);

	/* SW1A/1B standby voltage set to 0.975V */
	reg = 0xb;
	pmic_reg_write(p, PFUZE3000_SW1ASTBY, reg);
	pmic_reg_write(p, PFUZE3000_SW1BSTBY, reg);

	/* set SW1B normal voltage to 0.975V */
	pmic_reg_read(p, PFUZE3000_SW1BVOLT, &reg);
	reg &= ~0x1f;
	reg |= PFUZE3000_SW1AB_SETP(9750);
	pmic_reg_write(p, PFUZE3000_SW1BVOLT, reg);

	return 0;
}
#endif

int board_late_init(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_init();
#endif

	check_emmc();

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	return 0;
}

int checkboard(void)
{
	puts("Board: Variscite VAR-SOM-MX7\n");

	return 0;
}

#if defined(CONFIG_USB_EHCI_MX7) && !defined(CONFIG_DM_USB)
iomux_v3_cfg_t const usb_otg1_pads[] = {
	MX7D_PAD_GPIO1_IO05__USB_OTG1_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

iomux_v3_cfg_t const usb_otg2_pads[] = {
	MX7D_PAD_GPIO1_IO07__USB_OTG2_PWR | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_usb_phy_mode(int port)
{
	if (port == 1)
		return USB_INIT_HOST;

	return usb_phy_mode(port);
}

int board_ehci_hcd_init(int port)
{
	switch (port) {
	case 0:
		imx_iomux_v3_setup_multiple_pads(usb_otg1_pads,
						ARRAY_SIZE(usb_otg1_pads));
		break;
	case 1:
		imx_iomux_v3_setup_multiple_pads(usb_otg2_pads,
						ARRAY_SIZE(usb_otg2_pads));
		break;
	default:
		printf("MXC USB port %d not yet supported\n", port);
		return 1;
	}
	return 0;
}
#endif

#ifdef CONFIG_FSL_FASTBOOT

static void env_set_dev(char *var)
{
	char str[8];

	if (!env_check("dev_autodetect", "yes"))
		return;

#ifdef CONFIG_FASTBOOT_STORAGE_MMC
	sprintf(str, "mmc%d", mmc_get_env_dev());
	env_set(var, str);
#elif
	printf("unsupported boot device\n");
#endif
}

void add_soc_type_into_env(void);

void board_fastboot_setup(void)
{
	env_set_dev("fastboot_dev");
	env_set_dev("boota_dev");

	add_soc_type_into_env();
}

#ifdef CONFIG_ANDROID_RECOVERY

/* Use back key for recovery key */
#define GPIO_BACK_KEY IMX_GPIO_NR(1, 11)
iomux_v3_cfg_t const recovery_key_pads[] = {
	(MX7D_PAD_GPIO1_IO11__GPIO1_IO11 | MUX_PAD_CTRL(BUTTON_PAD_CTRL)),
};

int is_recovery_key_pressing(void)
{
	int button_pressed = 0;

	/* Check Recovery Combo Button press or not */
	imx_iomux_v3_setup_multiple_pads(recovery_key_pads,
			ARRAY_SIZE(recovery_key_pads));

	gpio_request(GPIO_BACK_KEY, "back_key");
	gpio_direction_input(GPIO_BACK_KEY);

	if (gpio_get_value(GPIO_BACK_KEY) == 0) { /* BACK key is low assert */
		button_pressed = 1;
		printf("Recovery key pressed\n");
	}

	return button_pressed;
}

void board_recovery_setup(void)
{
	env_set_dev("recovery_dev");

	printf("setup env for recovery...\n");
	env_set("bootcmd", "run bootcmd_android_recovery");
}
#endif /*CONFIG_ANDROID_RECOVERY*/

#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_SPL_BUILD
#include <spl.h>

#define CHECK_BITS_SET	0x80000000
#define END_OF_TABLE	0x00000000

static u32 default_dcd_table[] = {
	0x30340004, 0x4F400005,	/* Enable OCRAM EPDC */
	/* Clear then set bit30 to ensure exit from DDR retention */
	0x30360388, 0x40000000,
	0x30360384, 0x40000000,

	0x30391000, 0x00000002,	/* deassert presetn */
	/* ddrc */
	0x307a0000, 0x01040001,	/* mstr */
	0x307a01a0, 0x80400003,	/* dfiupd0 */
	0x307a01a4, 0x00100020,	/* dfiupd1 */
	0x307a01a8, 0x80100004,	/* dfiupd2 */
	0x307a0064, 0x00400046,	/* rfshtmg */
	0x307a0490, 0x00000001,	/* pctrl_0 */
	0x307a00d0, 0x00020083,	/* init0 */
	0x307a00d4, 0x00690000,	/* init1 */
	0x307a00dc, 0x09300004,	/* init3 */
	0x307a00e0, 0x04080000,	/* init4 */
	0x307a00e4, 0x00100004,	/* init5 */
	0x307a00f4, 0x0000033f,	/* rankctl */
	0x307a0100, 0x09081109,	/* dramtmg0 */
	0x307a0104, 0x0007020d,	/* dramtmg1 */
	0x307a0108, 0x03040407,	/* dramtmg2 */
	0x307a010c, 0x00002006,	/* dramtmg3 */
	0x307a0110, 0x04020205,	/* dramtmg4 */
	0x307a0114, 0x03030202,	/* dramtmg5 */
	0x307a0120, 0x00000803,	/* dramtmg8 */
	0x307a0180, 0x00800020,	/* zqctl0 */
	0x307a0190, 0x02098204,	/* dfitmg0 */
	0x307a0194, 0x00030303,	/* dfitmg1 */
	0x307a0200, 0x00000016,	/* addrmap0 */
	0x307a0204, 0x00080808,	/* addrmap1 */
	0x307a0210, 0x00000f0f,	/* addrmap4 */
	0x307a0214, 0x07070707,	/* addrmap5 */
	0x307a0218, 0x0F070707,	/* addrmap6 */
	0x307a0240, 0x06000604,	/* odtcfg */
	0x307a0244, 0x00000001,	/* odtmap */

	0x30391000, 0x00000000,	/* deassert presetn */

	/* ddr_phy */
	0x30790000, 0x17420f40,	/* phy_con0 */
	0x30790004, 0x10210100,	/* phy_con1 */
	0x30790010, 0x00060807,	/* phy_con4 */
	0x307900b0, 0x1010007e,	/* mdll_con0 */
	0x3079009c, 0x00000d6e,	/* drvds_con0 */
	0x30790020, 0x08080808,	/* offset_rd_con0 */
	0x30790030, 0x08080808,	/* offset_wr_con0 */
	0x30790050, 0x01000010,	/* cmd_sdll_con0 (OFFSETD_CON0) */
	0x30790050, 0x00000010,	/* cmd_sdll_con0 (OFFSETD_CON0) */
	0x307900c0, 0x0e407304,	/* zq_con0 */
	0x307900c0, 0x0e447304,	/* zq_con0 */
	0x307900c0, 0x0e447306,	/* zq_con0 */
	CHECK_BITS_SET, 0x307900c4, 0x1,
	0x307900c0, 0x0e447304,	/* zq_con0 */
	0x307900c0, 0x0e407304,	/* zq_con0 */

	0x30384130, 0x00000000,	/* Disable Clock */
	0x30340020, 0x00000178,	/* IOMUX_GRP_GRP8 - Start input to PHY */
	0x30384130, 0x00000002,	/* Enable Clock */
	0x30790018, 0x0000000f,	/* ddr_phy lp_con0 */

	CHECK_BITS_SET, 0x307a0004, 0x1,
};

static inline void check_bits_set(u32 reg, u32 mask)
{
	while ((readl(reg) & mask) != mask);
}

static inline bool var_eeprom_is_valid(const struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	return (EEPROM_MAGIC == p_var_eeprom_cfg->eeprom_magic);
}

static int var_eeprom_read_struct(struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	i2c_set_bus_num(EEPROM_I2C_BUS);
	if (i2c_probe(EEPROM_I2C_ADDR)) {
		eeprom_debug("\nError: Couldn't find EEPROM device\n");
		return -1;
	}

	if (i2c_read(EEPROM_I2C_ADDR, 0, 1,
				(u8 *) p_var_eeprom_cfg,
				sizeof(struct var_eeprom_cfg))) {
		eeprom_debug("\nError reading data from EEPROM\n");
		return -1;
	}

	if (!var_eeprom_is_valid(p_var_eeprom_cfg)) {
		eeprom_debug("\nError: Data on EEPROM is invalid\n");
		return -1;
	}

	return 0;
}

static void var_eeprom_print_legacy_production_info(const struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	/*
	 * Legacy EEPROM data layout:
	 * u32 reserved;
	 * u8 part_number[16];
	 * u8 assembly[16];
	 * u8 date[9];	YYYYMMMDD
	 */
	printf("\nPart number: %.*s\n",
			16,
			((char *) p_var_eeprom_cfg) + 4);

	printf("Assembly: %.*s\n",
			16,
			((char *) p_var_eeprom_cfg) + 4 + 16);

	printf("Date of production: %.*s %.*s %.*s\n",
			4, /* YYYY */
			((char *) p_var_eeprom_cfg) + 4 + 16 + 16,
			3, /* MMM */
			((char *) p_var_eeprom_cfg) + 4 + 16 + 16 + 4,
			2, /* DD */
			((char *) p_var_eeprom_cfg) + 4 + 16 + 16 + 4 + 3);
}

static void var_eeprom_print_production_info(const struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	printf("\nPart number: VSM-MX7-%.*s\n",
			sizeof(p_var_eeprom_cfg->part_number),
			(char *) p_var_eeprom_cfg->part_number);

	printf("Assembly: AS%.*s\n",
			sizeof(p_var_eeprom_cfg->assembly),
			(char *) p_var_eeprom_cfg->assembly);

	printf("Date of production: %.*s %.*s %.*s\n",
			4, /* YYYY */
			(char *) p_var_eeprom_cfg->date,
			3, /* MMM */
			((char *) p_var_eeprom_cfg->date) + 4,
			2, /* DD */
			((char *) p_var_eeprom_cfg->date) + 4 + 3);
}

static void ddr_init(u32 *table, int size)
{
	int i;

	for (i = 0; i < size; i += 2) {
		if (table[i] == CHECK_BITS_SET) {
			++i;
			eeprom_debug("check_bits_set(0x%x, 0x%x);\n", table[i], table[i + 1]);
			check_bits_set(table[i], table[i + 1]);
		} else if (table[i] == END_OF_TABLE) {
			break;
		} else {
			eeprom_debug("writel(0x%x, 0x%x);\n", table[i + 1], table[i]);
			writel(table[i + 1], table[i]);
		}
	}
}

static void set_ddr_freq_to_400mhz(void)
{
	struct mxc_ccm_anatop_reg *ccm_anatop = (struct mxc_ccm_anatop_reg *)
						 ANATOP_BASE_ADDR;
	struct mxc_ccm_reg *ccm_reg = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(CCM_ANALOG_PLL_DDR_POWERDOWN_MASK |
		CCM_ANALOG_PLL_DDR_TEST_DIV_SELECT_MASK |
		CCM_ANALOG_PLL_DDR_ENABLE_CLK_MASK |
		CCM_ANALOG_PLL_DDR_DIV2_ENABLE_CLK_MASK |
		(((0x21)<<CCM_ANALOG_PLL_DDR_DIV_SELECT_SHIFT)&CCM_ANALOG_PLL_DDR_DIV_SELECT_MASK),
		&ccm_anatop->pll_ddr);

	writel(0x0, &ccm_anatop->pll_ddr_num);

	writel(CCM_ANALOG_PLL_DDR_TEST_DIV_SELECT_MASK |
		CCM_ANALOG_PLL_DDR_ENABLE_CLK_MASK |
		CCM_ANALOG_PLL_DDR_DIV2_ENABLE_CLK_MASK |
		(((0x21)<<CCM_ANALOG_PLL_DDR_DIV_SELECT_SHIFT)&CCM_ANALOG_PLL_DDR_DIV_SELECT_MASK),
		&ccm_anatop->pll_ddr);

	check_bits_set((u32) &ccm_anatop->pll_ddr, CCM_ANALOG_PLL_DDR_LOCK_MASK);

	writel(0x1, &ccm_reg->root[DRAM_CLK_ROOT].target_root);
}

static void spl_dram_init(void)
{
	struct var_eeprom_cfg var_eeprom_cfg = {0};
	int is_eeprom_valid = !(var_eeprom_read_struct(&var_eeprom_cfg));

	/*
	 * Since the i.MX7D DDR controller doesn't
	 * support real calibration like the i.MX6,
	 * set the DDR freq. to 400MHz to be extra safe
	 */
	set_ddr_freq_to_400mhz();

	if (is_eeprom_valid) {
		var_eeprom_print_production_info(&var_eeprom_cfg);

		ddr_init(var_eeprom_cfg.dcd_table, ARRAY_SIZE(var_eeprom_cfg.dcd_table));
	} else {
		printf("\nUsing default DDR configuration");
		var_eeprom_print_legacy_production_info(&var_eeprom_cfg);

		ddr_init(default_dcd_table, ARRAY_SIZE(default_dcd_table));
	}
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	/* setup GP timer */
	timer_init();

	/* iomux and setup of i2c */
	board_early_init_f();

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}
#endif
