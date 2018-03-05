/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Copyright (C) 2016-2017 Variscite Ltd.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <malloc.h>
#include <micrel.h>
#include <miiphy.h>
#include <mmc.h>
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include <splash.h>
#include <usb.h>
#include <usb/ehci-ci.h>
#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/sys_proto.h>
#include <linux/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/video.h>
#include <asm/io.h>

#ifdef CONFIG_FSL_FASTBOOT
#include <fsl_fastboot.h>
#ifdef CONFIG_ANDROID_RECOVERY
#include <recovery.h>
#endif
#endif /*CONFIG_FSL_FASTBOOT*/

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)

#define PER_VCC_EN_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |	\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define I2C_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define OTG_ID_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)


#define VAR_SOM_BACKLIGHT_EN	IMX_GPIO_NR(4, 30)

bool lvds_enabled=false;

/*
 * Returns true iff the SOM is VAR-SOM-SOLO
 */
static bool is_som_solo(void)
{
	bool ret;
	int oldbus = i2c_get_bus_num();

	i2c_set_bus_num(PMIC_I2C_BUS);
	/* Probing for PMIC which is preset on all SOM types but SOM-SOLO */
	ret = (0 != i2c_probe(CONFIG_POWER_PFUZE100_I2C_ADDR));

	i2c_set_bus_num(oldbus);
	return ret;
}

/*
 * Returns true iff the carrier board is VAR-SOLOCustomBoard
 */
static bool is_solo_custom_board(void)
{
	bool ret;
	int oldbus = i2c_get_bus_num();

	i2c_set_bus_num(1);
	/* Probing for extra EEPROM present only on SOLOCustomBoard */
	ret = (0 == i2c_probe(0x51));

	i2c_set_bus_num(oldbus);
	return ret;
}

static bool is_cpu_pop_packaged(void)
{
	struct src *src_regs = (struct src *)SRC_BASE_ADDR;
	u32 soc_sbmr = readl(&src_regs->sbmr1);
	u8  boot_cfg3 = (soc_sbmr >> 16) & 0xFF;

	/* DDR Memory Map config == 4KB Interleaving Enabled */
	return ((boot_cfg3 & 0x30) == 0x20);
}

/*
 * Returns true iff the carrier board is VAR-DT6CustomBoard
 *  (and the SOM is DART-MX6)
 */
static inline bool is_dart_board(void)
{
	return is_cpu_pop_packaged();
}

/*
 * Returns true iff the carrier board is VAR-MX6CustomBoard
 */
static inline bool is_mx6_custom_board(void)
{
	return (!is_dart_board() && !is_solo_custom_board());
}

enum current_board {
	DART_BOARD,
	SOLO_CUSTOM_BOARD,
	MX6_CUSTOM_BOARD,
};

static enum current_board get_board_indx(void)
{
	if (is_dart_board())
		return DART_BOARD;
	if (is_solo_custom_board())
		return SOLO_CUSTOM_BOARD;
	if (is_mx6_custom_board())
		return MX6_CUSTOM_BOARD;

	printf("Error identifing carrier board!\n");
	hang();
}

enum mmc_boot_device {
	USDHC1,
	USDHC2,
	USDHC3,
	USDHC4,
};

int board_mmc_get_env_dev(int devno)
{
	if ((devno == USDHC1) || (devno == USDHC3))
		return 1; /* eMMC (non DART || DART) */
	else if (devno == USDHC2)
		return 0; /* SD card */
	else
		return -1;
}

static int check_env(char *var, char *val)
{
	char *read_val;
	if (var == NULL || val == NULL)
		return 0;

	read_val = getenv(var);

	if ((read_val != NULL) &&
			(strcmp(read_val, val) == 0)) {
		return 1;
	}

	return 0;
}

#ifdef CONFIG_SPLASH_SCREEN
static void set_splashsource_to_boot_rootfs(void)
{
	if (!check_env("splashsourceauto", "yes"))
		return;

#ifdef CONFIG_NAND_BOOT
	setenv("splashsource", getenv("rootfs_device"));
#else
	if (mmc_get_env_dev() == 0)
		setenv("splashsource", "sd");
	else if (mmc_get_env_dev() == 1)
		setenv("splashsource", "emmc");
#endif
}

int splash_screen_prepare(void)
{
	int ret=0;

	char sd_devpart_str[5];
	char emmc_devpart_str[5];
	u32 sd_part, emmc_part;

	sd_part = emmc_part = getenv_ulong("mmcrootpart", 10, 0);

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

	/* Turn on backlight */
	if (lvds_enabled)
		gpio_set_value(VAR_SOM_BACKLIGHT_EN, 1);

	return ret;
}
#endif

static iomux_v3_cfg_t const usdhc1_gpio_pads[] = {
	IOMUX_PADS(PAD_SD1_CLK__GPIO1_IO20	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_CMD__GPIO1_IO18	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT0__GPIO1_IO16	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT1__GPIO1_IO17	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT2__GPIO1_IO19	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT3__GPIO1_IO21	| MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static void print_emmc_size(void)
{
	struct mmc *mmc;
	int err;

	mmc = find_mmc_device(1);
	err = !mmc;
	if (!err) {
		/* Silence mmc_init since SOMs can be with or without eMMC */
		gd->flags |= GD_FLG_SILENT;
		err = mmc_init(mmc);
		gd->flags &= ~GD_FLG_SILENT;
	}

	if (err) {
		puts("No eMMC\n");
		if (!is_dart_board()) {
			/* VAR-SOM-MX6 rev 1.X externally exposes SD1 pins: */
			/* avoid any HW conflict configuring them as inputs */
			puts("Configuring SD1 pins as inputs\n");
			SETUP_IOMUX_PADS(usdhc1_gpio_pads);
			gpio_request(IMX_GPIO_NR(1, 16), "");
			gpio_request(IMX_GPIO_NR(1, 17), "");
			gpio_request(IMX_GPIO_NR(1, 18), "");
			gpio_request(IMX_GPIO_NR(1, 19), "");
			gpio_request(IMX_GPIO_NR(1, 20), "");
			gpio_request(IMX_GPIO_NR(1, 21), "");
			gpio_direction_input(IMX_GPIO_NR(1, 16));
			gpio_direction_input(IMX_GPIO_NR(1, 17));
			gpio_direction_input(IMX_GPIO_NR(1, 18));
			gpio_direction_input(IMX_GPIO_NR(1, 19));
			gpio_direction_input(IMX_GPIO_NR(1, 20));
			gpio_direction_input(IMX_GPIO_NR(1, 21));
		}
		return;
	}

	puts("eMMC:  ");
	print_size(mmc->capacity, "\n");
}

/*
 * Returns DRAM size in MiB
 */
static u32 var_get_ram_size(void)
{
	u32 *p_ram_size = (u32 *)RAM_SIZE_ADDR;
	return *p_ram_size;
}

int dram_init(void)
{
	gd->ram_size = var_get_ram_size() * 1024 * 1024;
	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	IOMUX_PADS(PAD_CSI0_DAT10__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_CSI0_DAT11__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL)),
};

static iomux_v3_cfg_t const enet_pads1[] = {
	IOMUX_PADS(PAD_ENET_MDIO__ENET_MDIO	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_MDC__ENET_MDC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TXC__RGMII_TXC	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD0__RGMII_TD0	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD1__RGMII_TD1	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD2__RGMII_TD2	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TD3__RGMII_TD3	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	/* pin 35 - 1 (PHY_AD2) on reset */
	IOMUX_PADS(PAD_RGMII_RXC__GPIO6_IO30		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 32 - 1 - (MODE0) all */
	IOMUX_PADS(PAD_RGMII_RD0__GPIO6_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 31 - 1 - (MODE1) all */
	IOMUX_PADS(PAD_RGMII_RD1__GPIO6_IO27		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 28 - 1 - (MODE2) all */
	IOMUX_PADS(PAD_RGMII_RD2__GPIO6_IO28		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 27 - 1 - (MODE3) all */
	IOMUX_PADS(PAD_RGMII_RD3__GPIO6_IO29		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
	IOMUX_PADS(PAD_RGMII_RX_CTL__GPIO6_IO24		| MUX_PAD_CTRL(NO_PAD_CTRL)),
	/* PHY Reset */
	IOMUX_PADS(PAD_ENET_CRS_DV__GPIO1_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL)),
};

static iomux_v3_cfg_t const enet_pads2[] = {
	IOMUX_PADS(PAD_RGMII_RXC__RGMII_RXC		| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD0__RGMII_RD0		| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD1__RGMII_RD1		| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD2__RGMII_RD2		| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RD3__RGMII_RD3		| MUX_PAD_CTRL(ENET_PAD_CTRL)),
	IOMUX_PADS(PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL)),
};

static void setup_iomux_enet(void)
{
	gpio_request(IMX_GPIO_NR(1, 25), "ENET PHY Reset");
	gpio_direction_output(IMX_GPIO_NR(1, 25), 0);

	gpio_request(IMX_GPIO_NR(6, 30), "");
	gpio_request(IMX_GPIO_NR(6, 25), "");
	gpio_request(IMX_GPIO_NR(6, 27), "");
	gpio_request(IMX_GPIO_NR(6, 28), "");
	gpio_request(IMX_GPIO_NR(6, 29), "");
	gpio_direction_output(IMX_GPIO_NR(6, 30), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 25), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 27), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 28), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 29), 1);

	SETUP_IOMUX_PADS(enet_pads1);

	gpio_request(IMX_GPIO_NR(6, 24), "");
	gpio_direction_output(IMX_GPIO_NR(6, 24), 1);

	mdelay(10);
	/* Reset PHY */
	gpio_set_value(IMX_GPIO_NR(1, 25), 1);

	SETUP_IOMUX_PADS(enet_pads2);
}

static iomux_v3_cfg_t const usdhc1_pads[] = {
	IOMUX_PADS(PAD_SD1_CLK__SD1_CLK		| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_CMD__SD1_CMD		| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT0__SD1_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT1__SD1_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT2__SD1_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD1_DAT3__SD1_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	IOMUX_PADS(PAD_SD2_CLK__SD2_CLK		| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_CMD__SD2_CMD		| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT0__SD2_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT1__SD2_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT2__SD2_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD2_DAT3__SD2_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};

#ifndef CONFIG_SPL_BUILD
static iomux_v3_cfg_t const usdhc2_cd_pad[][1*2] = {
	{
		/* DART */
		IOMUX_PADS(PAD_GPIO_6__GPIO1_IO06 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	},
	{
		/* Non-DART */
		IOMUX_PADS(PAD_KEY_COL4__GPIO4_IO14 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	}
};
#endif

static iomux_v3_cfg_t const usdhc3_pads[] = {
	IOMUX_PADS(PAD_SD3_CLK__SD3_CLK		| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_CMD__SD3_CMD		| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT0__SD3_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT1__SD3_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT2__SD3_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
	IOMUX_PADS(PAD_SD3_DAT3__SD3_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL)),
};

#ifdef CONFIG_SYS_I2C_MXC
I2C_PADS(i2c_pad_info1,
	PAD_CSI0_DAT9__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_CSI0_DAT9__GPIO5_IO27 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(5, 27),
	PAD_CSI0_DAT8__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_CSI0_DAT8__GPIO5_IO26 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(5, 26));

I2C_PADS(i2c_pad_info2,
	PAD_KEY_COL3__I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_KEY_COL3__GPIO4_IO12 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(4, 12),
	PAD_KEY_ROW3__I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_KEY_ROW3__GPIO4_IO13 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(4, 13));

I2C_PADS(i2c_pad_info3,
	PAD_GPIO_5__I2C3_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_GPIO_5__GPIO1_IO05 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(1, 5),
	PAD_GPIO_16__I2C3_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_GPIO_16__GPIO7_IO11 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(7, 11));
#endif

static void setup_local_i2c(void)
{
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info1));
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info2));
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info3));
}

static void setup_iomux_uart(void)
{
	SETUP_IOMUX_PADS(uart1_pads);
}

#ifdef CONFIG_FSL_ESDHC
static struct fsl_esdhc_cfg usdhc_cfg[2];

#ifdef CONFIG_ENV_IS_IN_MMC
int mmc_map_to_kernel_blk(int dev_no)
{
	if ((!is_dart_board()) && (dev_no == 1))
		return 0;
	return dev_no + 1;
}
#endif

static int usdhc2_cd_gpio[] = {
	/* DART */
	IMX_GPIO_NR(1, 6),
	/* Non-DART */
	IMX_GPIO_NR(4, 14)
};

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int board = is_dart_board() ? 0 : 1;

	/* SD card */
	if (cfg->esdhc_base == USDHC2_BASE_ADDR) {
		return !gpio_get_value(usdhc2_cd_gpio[board]);
	}

	/*
	 * On DART SOMs eMMC is always present.
	 *
	 * On non DART SOMs eMMC can be present or not,
	 * but we can't know until we try to init it
	 * so return 1 here anyway
	 */
	return 1;
}

#ifdef CONFIG_SPL_BUILD
static enum mmc_boot_device get_mmc_boot_device(void)
{
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned reg = readl(&psrc->sbmr1) >> 11;
	/*
	 * Upon reading BOOT_CFG register
	 * Bit 11 and 12 of BOOT_CFG register can determine the current
	 * mmc port
	 */

	return (reg & 0x3);
}
#endif

int board_mmc_init(bd_t *bis)
{
#ifndef CONFIG_SPL_BUILD
	int ret, i, board;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    SD2 (SD)
	 *
	 * On non DART boards:
	 * mmc1                    SD1 (eMMC)
	 *
	 * On DART board:
	 * mmc1                    SD3 (eMMC)
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			SETUP_IOMUX_PADS(usdhc2_pads);

			board = is_dart_board() ? 0 : 1;
			SETUP_IOMUX_PADS(usdhc2_cd_pad[board]);
			gpio_request(usdhc2_cd_gpio[board], "USDHC2 CD");
			gpio_direction_input(usdhc2_cd_gpio[board]);

			usdhc_cfg[0].esdhc_base = USDHC2_BASE_ADDR;
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			usdhc_cfg[0].max_bus_width = 4;
			break;
		case 1:
			if (is_dart_board()) {
				SETUP_IOMUX_PADS(usdhc3_pads);
				usdhc_cfg[1].esdhc_base = USDHC3_BASE_ADDR;
				usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			} else {
				SETUP_IOMUX_PADS(usdhc1_pads);
				usdhc_cfg[1].esdhc_base = USDHC1_BASE_ADDR;
				usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			}
			usdhc_cfg[1].max_bus_width = 4;
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
			       "(%d) then supported by the board (%d)\n",
			       i + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}
	return 0;
#else
	/*
	 * Possible MMC boot devices:
	 * SD1 (eMMC on non DART boards)
	 * SD2 (SD)
	 * SD3 (eMMC on DART board)
	 */
	puts("MMC Boot Device: ");
	switch (get_mmc_boot_device()) {
	case USDHC1:
		puts("mmc1 (eMMC)");
		SETUP_IOMUX_PADS(usdhc1_pads);
		usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		usdhc_cfg[0].max_bus_width = 4;
		break;
	case USDHC2:
		puts("mmc0 (SD)");
		SETUP_IOMUX_PADS(usdhc2_pads);
		usdhc_cfg[0].esdhc_base = USDHC2_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		usdhc_cfg[0].max_bus_width = 4;
		break;
	case USDHC3:
		puts("mmc1 (eMMC)");
		SETUP_IOMUX_PADS(usdhc3_pads);
		usdhc_cfg[0].esdhc_base = USDHC3_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
		gd->arch.sdhc_clk = usdhc_cfg[0].sdhc_clk;
		usdhc_cfg[0].max_bus_width = 4;
		break;
	default:
		break;
	}
	puts("\n");

	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
#endif
}
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
static void mmc_late_init(void)
{
	char cmd[32];
	u32 dev_no = mmc_get_env_dev();

	if (!check_env("mmcautodetect", "yes"))
		return;

	setenv_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	setenv_ulong("mmcblk", mmc_map_to_kernel_blk(dev_no));

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}
#endif

#ifdef CONFIG_NAND_MXS
static iomux_v3_cfg_t const gpmi_pads[] = {
	IOMUX_PADS(PAD_NANDF_CLE__NAND_CLE	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_ALE__NAND_ALE	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_WP_B__NAND_WP_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_RB0__NAND_READY_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL0)),
	IOMUX_PADS(PAD_NANDF_CS0__NAND_CE0_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_SD4_CMD__NAND_RE_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_SD4_CLK__NAND_WE_B	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D0__NAND_DATA00	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D1__NAND_DATA01	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D2__NAND_DATA02	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D3__NAND_DATA03	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D4__NAND_DATA04	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D5__NAND_DATA05	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D6__NAND_DATA06	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_NANDF_D7__NAND_DATA07	| MUX_PAD_CTRL(GPMI_PAD_CTRL2)),
	IOMUX_PADS(PAD_SD4_DAT0__NAND_DQS	| MUX_PAD_CTRL(GPMI_PAD_CTRL1)),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	SETUP_IOMUX_PADS(gpmi_pads);

	/* gate ENFC_CLK_ROOT clock first,before clk source switch */
	clrbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* config gpmi and bch clock to 100 MHz */
	clrsetbits_le32(&mxc_ccm->cs2cdr,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF(0) |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED(3) |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL(3));

	/* enable ENFC_CLK_ROOT clock */
	setbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
			MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
			MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
			MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
			MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
			MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_OFFSET);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

int board_phy_config(struct phy_device *phydev)
{
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	/* manually configure PHY as master during master-slave negotiation */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x9, 0x1c00);

	/* control data pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_CTRL_SIG_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* rx data pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_RX_DATA_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* tx data pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_TX_DATA_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x0000);
	/* gtx and rx clock pad skew */
	ksz9031_phy_extended_write(phydev, 0x02,
			MII_KSZ9031_EXT_RGMII_CLOCK_SKEW,
			MII_KSZ9031_MOD_DATA_NO_POST_INC, 0x03FF);

	return 0;
}

#if defined(CONFIG_VIDEO_IPUV3)
static void disable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;

	int reg = readl(&iomux->gpr[2]);

	reg &= ~(IOMUXC_GPR2_LVDS_CH0_MODE_MASK |
		 IOMUXC_GPR2_LVDS_CH1_MODE_MASK);

	writel(reg, &iomux->gpr[2]);
}

static void do_enable_hdmi(struct display_info_t const *dev)
{
	disable_lvds(dev);
	/*
	 * imx_enable_hdmi_phy(); should be called here.
	 * It is ommitted to avoid a current known bug where
	 * the boot sometimes hangs if an HDMI cable is attached
	 * - at least on some SOMs.
	 */
}

static void lvds_enable_disable(struct display_info_t const *dev)
{
	if (getenv("splashimage") != NULL)
		lvds_enabled=true;
	else
		disable_lvds(dev);
}

static int detect_dart_vsc_display(struct display_info_t const *dev)
{
	return (!is_mx6_custom_board());
}

static int detect_mx6cb_cdisplay(struct display_info_t const *dev)
{
	if (!is_mx6_custom_board())
		return 0;

	i2c_set_bus_num(dev->bus);
	return (0 == i2c_probe(dev->addr));
}

static int detect_mx6cb_rdisplay(struct display_info_t const *dev)
{
	if (!is_mx6_custom_board())
		return 0;

	/* i2c probe the *c*display */
	i2c_set_bus_num(MX6CB_CDISPLAY_I2C_BUS);
	return (0 != i2c_probe(MX6CB_CDISPLAY_I2C_ADDR));
}

#define MHZ2PS(f)	(1000000/(f))

struct display_info_t const displays[] = {{
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_hdmi,
	.enable	= do_enable_hdmi,
	.mode	= {
		.name           = "HDMI",
		.refresh        = 60,  /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 31777,
		.left_margin    = 48,
		.right_margin   = 16,
		.upper_margin   = 33,
		.lower_margin   = 10,
		.hsync_len      = 96,
		.vsync_len      = 2,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB666,
	.detect	= detect_dart_vsc_display,
	.enable	= lvds_enable_disable,
	.mode	= {
		.name           = "VAR-WVGA",
		.refresh        = 60,  /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(50),
		.left_margin    = 40,
		.right_margin   = 40,
		.upper_margin   = 29,
		.lower_margin   = 13,
		.hsync_len      = 48,
		.vsync_len      = 3,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= MX6CB_CDISPLAY_I2C_BUS,
	.addr	= MX6CB_CDISPLAY_I2C_ADDR,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_mx6cb_cdisplay,
	.enable	= lvds_enable_disable,
	.mode	= {
		.name           = "VAR-WVGA MX6CB-C",
		.refresh        = 60,  /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(50),
		.left_margin    = 39,
		.right_margin   = 39,
		.upper_margin   = 29,
		.lower_margin   = 13,
		.hsync_len      = 128,
		.vsync_len      = 2,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= detect_mx6cb_rdisplay,
	.enable	= lvds_enable_disable,
	.mode	= {
		.name           = "VAR-WVGA MX6CB-R",
		.refresh        = 60,  /* optional */
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(50),
		.left_margin    = 0,
		.right_margin   = 40,
		.upper_margin   = 20,
		.lower_margin   = 13,
		.hsync_len      = 48,
		.vsync_len      = 3,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);

static void setup_display(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int reg;

	/* Setup backlight */
	SETUP_IOMUX_PAD(PAD_DISP0_DAT9__GPIO4_IO30 | MUX_PAD_CTRL(NO_PAD_CTRL));

	/* Turn off backlight until display is ready */
	gpio_request(VAR_SOM_BACKLIGHT_EN, "Display Backlight Enable");
	gpio_direction_output(VAR_SOM_BACKLIGHT_EN , 0);

	enable_ipu_clock();
	imx_setup_hdmi();

	/* Turn on LDB0, LDB1, IPU,IPU DI0 clocks */
	reg = readl(&mxc_ccm->CCGR3);
	reg |=  MXC_CCM_CCGR3_LDB_DI0_MASK | MXC_CCM_CCGR3_LDB_DI1_MASK;
	writel(reg, &mxc_ccm->CCGR3);

	/* set LDB0, LDB1 clk select to 011/011 */
	reg = readl(&mxc_ccm->cs2cdr);
	reg &= ~(MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK
		 | MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK);
	/* 1 -> ~50MHz , 2 -> ~56MHz, 3 -> ~75MHz, 4 -> ~68MHz */
	reg |= (1 << MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET)
	      | (1 << MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->cs2cdr);

	reg = readl(&mxc_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV | MXC_CCM_CSCMR2_LDB_DI1_IPU_DIV;
	writel(reg, &mxc_ccm->cscmr2);

	reg = readl(&mxc_ccm->chsccdr);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<< MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_OFFSET);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0
		<< MXC_CCM_CHSCCDR_IPU1_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);

	reg = IOMUXC_GPR2_BGREF_RRMODE_EXTERNAL_RES
	     | IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_LOW
	     | IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW
	     | IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG
	     | IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT
	     | IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG
	     | IOMUXC_GPR2_DATA_WIDTH_CH0_24BIT
	     | IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0
	     | IOMUXC_GPR2_LVDS_CH1_MODE_ENABLED_DI0;
	writel(reg, &iomux->gpr[2]);

	reg = readl(&iomux->gpr[3]);
	reg = (reg & ~(IOMUXC_GPR3_LVDS1_MUX_CTL_MASK | IOMUXC_GPR3_HDMI_MUX_CTL_MASK))
		| (IOMUXC_GPR3_MUX_SRC_IPU1_DI0 << IOMUXC_GPR3_LVDS1_MUX_CTL_OFFSET);
	writel(reg, &iomux->gpr[3]);
}
#elif defined(CONFIG_IMX_HDMI)
static void setup_hdmi(void)
{
	/* Turn off standard backlight: pin as input pull down */
	SETUP_IOMUX_PAD(PAD_DISP0_DAT9__GPIO4_IO30 | MUX_PAD_CTRL(PAD_CTL_PUS_100K_DOWN));
	gpio_request(VAR_SOM_BACKLIGHT_EN, "Display Backlight Enable");
	gpio_direction_input(VAR_SOM_BACKLIGHT_EN);

	imx_setup_hdmi();
}
#endif /* CONFIG_VIDEO_IPUV3 */

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}

int board_eth_init(bd_t *bis)
{
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret;

	setup_iomux_enet();

#ifdef CONFIG_FEC_MXC
	bus = fec_get_miibus(base, -1);
	if (!bus) {
		printf("FEC MXC bus: %s:failed\n", __func__);
		return 0;
	}

	phydev = phy_find_by_mask(bus, (0x1 << CONFIG_FEC_MXC_PHYADDR), PHY_INTERFACE_MODE_RGMII);
	if (!phydev) {
		printf("FEC MXC phy find: %s:failed\n", __func__);
		free(bus);
		return 0;
	}
	printf("using phy at %d\n", phydev->addr);

	ret  = fec_probe(bis, -1, base, bus, phydev);
	if (ret) {
		printf("FEC MXC probe: %s:failed\n", __func__);
		free(phydev);
		free(bus);
	}
#endif

#if defined(CONFIG_CI_UDC) && defined(CONFIG_USB_ETHER)
	/* USB Ethernet Gadget */
	usb_eth_initialize(bis);
#endif

	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB
static int const usb_otg_pwr_en_gpio[] = {
	/* DART */
	IMX_GPIO_NR(4, 15),
	/* SOLOCustomBoard */
	IMX_GPIO_NR(3, 22),
};

static int const usb_h1_pwr_en_gpio[] = {
	/* DART */
	IMX_GPIO_NR(1, 28),
	/* SOLOCustomBoard */
	IMX_GPIO_NR(4, 15),
};

static iomux_v3_cfg_t const usb_pads[][3*2] = {
	{
		/* DART */
		IOMUX_PADS(PAD_ENET_RX_ER__USB_OTG_ID	| MUX_PAD_CTRL(OTG_ID_PAD_CTRL)),
		IOMUX_PADS(PAD_KEY_ROW4__GPIO4_IO15	| MUX_PAD_CTRL(NO_PAD_CTRL)),
		IOMUX_PADS(PAD_ENET_TX_EN__GPIO1_IO28	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	},
	{
		/* SOLOCustomBoard */
		IOMUX_PADS(PAD_GPIO_1__USB_OTG_ID	| MUX_PAD_CTRL(OTG_ID_PAD_CTRL)),
		IOMUX_PADS(PAD_EIM_D22__GPIO3_IO22	| MUX_PAD_CTRL(NO_PAD_CTRL)),
		IOMUX_PADS(PAD_KEY_ROW4__GPIO4_IO15	| MUX_PAD_CTRL(NO_PAD_CTRL)),
	}
};

static void setup_usb(void)
{
	int board = get_board_indx();
	if (board == MX6_CUSTOM_BOARD)
		return;

	SETUP_IOMUX_PADS(usb_pads[board]);
	gpio_request(usb_otg_pwr_en_gpio[board], "USB OTG Power Enable");
	gpio_request(usb_h1_pwr_en_gpio[board], "USB H1 Power Enable");
	gpio_direction_output(usb_otg_pwr_en_gpio[board], 0);
	gpio_direction_output(usb_h1_pwr_en_gpio[board], 0);
}

int board_usb_phy_mode(int port)
{
	if (is_mx6_custom_board() && port == 0) {
		if (check_env("usbmode", "host"))
			return USB_INIT_HOST;
		else
			return USB_INIT_DEVICE;
	}
	return usb_phy_mode(port);
}

int board_ehci_power(int port, int on)
{
	int board = get_board_indx();
	if (board == MX6_CUSTOM_BOARD)
		return 0; /* no power enable needed */

	if (port > 1)
		return -EINVAL;

	if (port)
		gpio_set_value(usb_h1_pwr_en_gpio[board], on);
	else
		gpio_set_value(usb_otg_pwr_en_gpio[board], on);

	return 0;
}
#endif
#endif /* CONFIG_USB_EHCI_MX6 */

int board_early_init_f(void)
{
	setup_iomux_uart();
#ifdef CONFIG_SYS_I2C_MXC
	setup_local_i2c();
#endif
#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

	return 0;
}

int board_init(void)
{
#if defined(CONFIG_VIDEO_IPUV3)
	setup_display();
#elif defined(CONFIG_IMX_HDMI)
	setup_hdmi();
#endif

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB
	setup_usb();
#endif
	int board = get_board_indx();

	/* 'usb_otg_id' pin iomux select control */
	if (board == DART_BOARD)
		imx_iomux_set_gpr_register(1, 13, 1, 0);
	else if (board == SOLO_CUSTOM_BOARD)
		imx_iomux_set_gpr_register(1, 13, 1, 1);
#endif
	return 0;
}

static struct pmic *pfuze;

struct pmic_write_values {
	u32 reg;
	u32 mask;
	u32 writeval;
};

static int pmic_write_val(struct pmic *p, struct pmic_write_values pmic_struct)
{
	unsigned int val = 0;
	int retval = 0;

	if (!p) {
		printf("No PMIC found!\n");
		return -1;
	}

	retval += pmic_reg_read(p, pmic_struct.reg, &val);
	val &= ~(pmic_struct.mask);
	val |= pmic_struct.writeval;
	retval += pmic_reg_write(p, pmic_struct.reg, val);

	if (retval)
		printf("PMIC write voltages error!\n");

	return retval;
}

static int pmic_write_vals(struct pmic *p, struct pmic_write_values *arr, int arr_size)
{
	int i, retval;

	for (i = 0; i < arr_size; ++i) {
		retval = pmic_write_val(p, arr[i]);
		if (retval)
			return retval;
	}

	return 0;
}

#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	if (!is_som_solo()) {
		struct pmic *p = pfuze;

		struct pmic_write_values ldo_mode_arr[] = {
			/* Set SW1AB to 1.325V */
			{PFUZE100_SW1ABVOL, SW1x_NORMAL_MASK, SW1x_1_325V},
			/* Set SW1C to 1.325V */
			{PFUZE100_SW1CVOL, SW1x_NORMAL_MASK, SW1x_1_325V}
		};

		if (pmic_write_vals(p, ldo_mode_arr, ARRAY_SIZE(ldo_mode_arr)))
			return;

		set_anatop_bypass(0);
		printf("switch to ldo_bypass mode!\n");
	}
}
#endif

static int pfuze_mode_init(struct pmic *p, u32 mode)
{
	unsigned char offset, i, switch_num;
	u32 id;
	int ret;

	pmic_reg_read(p, PFUZE100_DEVICEID, &id);
	id = id & 0xf;

	if (id == 0) {
		switch_num = 6;
		offset = PFUZE100_SW1CMODE;
	} else if (id == 1) {
		switch_num = 4;
		offset = PFUZE100_SW2MODE;
	} else {
		printf("Not supported, id=%d\n", id);
		return -EINVAL;
	}

	ret = pmic_reg_write(p, PFUZE100_SW1ABMODE, mode);
	if (ret < 0) {
		printf("Set SW1AB mode error!\n");
		return ret;
	}

	for (i = 0; i < switch_num - 1; i++) {
		ret = pmic_reg_write(p, offset + i * SWITCH_SIZE, mode);
		if (ret < 0) {
			printf("Set switch 0x%x mode error!\n",
			       offset + i * SWITCH_SIZE);
			return ret;
		}
	}

	return ret;
}

int power_init_board(void)
{
	if (!is_som_solo()) {
		unsigned int reg;
		int retval;

		retval = power_pfuze100_init(PMIC_I2C_BUS);
		if (retval)
			return -ENODEV;
		pfuze = pmic_get("PFUZE100");
		retval = pmic_probe(pfuze);
		if (retval)
			return -ENODEV;
		pmic_reg_read(pfuze, PFUZE100_DEVICEID, &reg);
		printf("PMIC:  PFUZE100 ID=0x%02x\n", reg);

		if (is_dart_board()) {

			struct pmic_write_values dart_pmic_arr[] = {
				/* Set SW1AB standby volage to 0.9V */
				{PFUZE100_SW1ABSTBY, SW1x_STBY_MASK, SW1x_0_900V},

				/* Set SW1AB off volage to 0.9V */
				{PFUZE100_SW1ABOFF, SW1x_OFF_MASK, SW1x_0_900V},

				/* Set SW1C standby voltage to 0.9V */
				{PFUZE100_SW1CSTBY, SW1x_STBY_MASK, SW1x_0_900V},

				/* Set SW1C off volage to 0.9V */
				{PFUZE100_SW1COFF, SW1x_OFF_MASK, SW1x_0_900V},

				/* Set SW2 to 3.3V */
				{PFUZE100_SW2VOL, SWx_NORMAL_MASK, SWx_HR_3_300V},

				/* Set SW2 standby voltage to 3.2V */
				{PFUZE100_SW2STBY, SWx_STBY_MASK, SWx_HR_3_200V},

				/* Set SW2 off voltage to 3.2V */
				{PFUZE100_SW2OFF, SWx_OFF_MASK, SWx_HR_3_200V},

				/* Set SW1AB/VDDARM step ramp up time 2us */
				{PFUZE100_SW1ABCONF, SW1xCONF_DVSSPEED_MASK, SW1xCONF_DVSSPEED_2US},

				/* Set SW1AB, SW1C, SW2 normal mode to PWM, and standby mode to PFM */
				{PFUZE100_SW1ABMODE, SW_MODE_MASK, PWM_PFM},
				{PFUZE100_SW1CMODE, SW_MODE_MASK, PWM_PFM},
				{PFUZE100_SW2MODE, SW_MODE_MASK, PWM_PFM},

				/* Set VGEN6 to 3.3V */
				{PFUZE100_VGEN6VOL, LDO_VOL_MASK, LDOB_3_30V}
			};

			retval = pmic_write_vals(pfuze, dart_pmic_arr, ARRAY_SIZE(dart_pmic_arr));

		} else {

			struct pmic_write_values pmic_arr[] = {
				/* Set SW1AB standby volage to 0.975V */
				{PFUZE100_SW1ABSTBY, SW1x_STBY_MASK, SW1x_0_975V},

				/* Set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
				{PFUZE100_SW1ABCONF, SW1xCONF_DVSSPEED_MASK, SW1xCONF_DVSSPEED_4US},

				/* Set SW1C standby voltage to 0.975V */
				{PFUZE100_SW1CSTBY, SW1x_STBY_MASK, SW1x_0_975V},

				/* Set Gigbit Ethernet voltage */
				{PFUZE100_SW4VOL, SWx_NORMAL_MASK, SWx_LR_1_200V},

				/* Increase VGEN5 from 2.8 to 3V */
				{PFUZE100_VGEN5VOL, LDO_VOL_MASK, LDOB_3_00V},

				/* Set VGEN3 to 2.5V */
				{PFUZE100_VGEN3VOL, LDO_VOL_MASK, LDOB_2_50V},
#ifdef LOW_POWER_MODE_ENABLE
				/* Set low power mode voltages to disable */

				/* SW2 already set to 3.2V in SPL */

				/* Set SW3A standby voltage to 1.275V */
				{PFUZE100_SW3ASTBY, SWx_STBY_MASK, SWx_LR_1_275V},

				/* Set SW3A off voltage to 1.275V */
				{PFUZE100_SW3AOFF, SWx_OFF_MASK, SWx_LR_1_275V},

				/* SW4MODE = OFF in standby */
				{PFUZE100_SW4MODE, SW_MODE_MASK, PWM_OFF},

				/* Set VGEN4CTL = low power in standby */
				{PFUZE100_VGEN4VOL, (LDO_MODE_MASK | LDO_EXT_MODE_MASK), \
					((LDO_MODE_ON << LDO_MODE_SHIFT) | LDO_EXT_MODE_ON_LPM << LDO_EXT_MODE_SHIFT)},

				/* Set VGEN6CTL = OFF in standby */
				{PFUZE100_VGEN6VOL, (LDO_MODE_MASK | LDO_EXT_MODE_MASK), \
					((LDO_MODE_ON << LDO_MODE_SHIFT) | LDO_EXT_MODE_ON_OFF << LDO_EXT_MODE_SHIFT)},

				/* Set VGEN3CTL = low power */
				{PFUZE100_VGEN3VOL, (LDO_MODE_MASK | LDO_EXT_MODE_MASK), \
					((LDO_MODE_ON << LDO_MODE_SHIFT) | LDO_EXT_MODE_LPM_LPM << LDO_EXT_MODE_SHIFT)}
#else
				/* Set VGEN3CTL = low power in standby */
				{PFUZE100_VGEN3VOL, (LDO_MODE_MASK | LDO_EXT_MODE_MASK), \
					((LDO_MODE_ON << LDO_MODE_SHIFT) | LDO_EXT_MODE_ON_LPM << LDO_EXT_MODE_SHIFT)}
#endif
			};

			if (is_mx6dqp())
				pfuze_mode_init(pfuze, APS_APS);

			retval = pmic_write_vals(pfuze, pmic_arr, ARRAY_SIZE(pmic_arr));
		}

		/* Set SW1C/VDDSOC step ramp up time from 16us to 4us/25mV */
		retval += pmic_write_val(pfuze,
				(struct pmic_write_values)
				{PFUZE100_SW1CCONF, SW1xCONF_DVSSPEED_MASK, SW1xCONF_DVSSPEED_4US});

		if (retval)
			return retval;
	}

	return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
	mmc_late_init();
#endif
	print_emmc_size();

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (is_dart_board())
		setenv("board_name", "DT6CUSTOM");
	else if (is_solo_custom_board())
		setenv("board_name", "SOLOCUSTOM");
	else
		setenv("board_name", "MX6CUSTOM");

	if (is_som_solo())
		setenv("board_som", "SOM-SOLO");
	else if (is_dart_board())
		setenv("board_som", "DART-MX6");
	else
		setenv("board_som", "SOM-MX6");

	if (is_mx6dqp())
		setenv("board_rev", "MX6QP");
	else if (is_mx6dq())
		setenv("board_rev", "MX6Q");
	else if (is_mx6sdl())
		setenv("board_rev", "MX6DL");
#endif

	return 0;
}

int checkboard(void)
{
	puts("Board: Variscite ");

	if (is_som_solo())
		puts("VAR-SOM-SOLO\n");
	else if (is_dart_board())
		puts("DART-MX6\n");
	else
		puts("VAR-SOM-MX6\n");

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT

#ifdef CONFIG_NAND_BOOT
int mmc_get_env_dev()
{
	return 1;
}
#endif

static void setenv_dev(char *var)
{
	char str[8];

	if (!check_env("dev_autodetect", "yes"))
		return;

	sprintf(str, "mmc%d", mmc_get_env_dev());
	setenv(var, str);
}

void add_soc_type_into_env(void);

void board_fastboot_setup(void)
{
	setenv_dev("fastboot_dev");
	setenv_dev("boota_dev");

	add_soc_type_into_env();
}

#ifdef CONFIG_ANDROID_RECOVERY

static int back_key_gpio[] = {
	/* DART */
	IMX_GPIO_NR(4, 26),
	/* Non-DART */
	IMX_GPIO_NR(5, 20)
};

static iomux_v3_cfg_t const back_key_pad[][1*2] = {
	{
		/* DART */
		IOMUX_PADS(PAD_DISP0_DAT5__GPIO4_IO26 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	},
	{
		/* Non-DART */
		IOMUX_PADS(PAD_CSI0_DATA_EN__GPIO5_IO20 | MUX_PAD_CTRL(NO_PAD_CTRL)),
	}
};

int is_recovery_key_pressing(void)
{
	int button_pressed = 0;
	int board = is_dart_board() ? 0 : 1;

	/* Check Recovery Combo Button press or not. */
	SETUP_IOMUX_PADS(back_key_pad[board]);

	gpio_request(back_key_gpio[board], "Back key");
	gpio_direction_input(back_key_gpio[board]);

	if (gpio_get_value(back_key_gpio[board]) == 0) { /* BACK key is low assert */
		button_pressed = 1;
		printf("Recovery key pressed\n");
	}

	return button_pressed;
}

void board_recovery_setup(void)
{
	setenv_dev("recovery_dev");

	printf("setup env for recovery...\n");
	setenv("bootcmd", "run bootcmd_android_recovery");
}

#endif /*CONFIG_ANDROID_RECOVERY*/

#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <libfdt.h>
#include <asm/arch/mx6-ddr.h>
#include "mx6var_dram.h"

#ifdef CONFIG_SPL_OS_BOOT
int spl_start_uboot(void)
{
	return 0;
}
#endif

/*
 * Writes RAM size (MiB) to RAM_SIZE_ADDR so U-Boot can read it
 */
void var_set_ram_size(u32 ram_size)
{
	u32 *p_ram_size = (u32 *)RAM_SIZE_ADDR;
	if (ram_size > 3840)
		ram_size = 3840;
	*p_ram_size = ram_size;
}

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(0x00C03F3F, &ccm->CCGR0);
	writel(0x0030FC03, &ccm->CCGR1);
	writel(0x0FFFC000, &ccm->CCGR2);
	writel(0x3FF00000, &ccm->CCGR3);
	writel(0x00FFF300, &ccm->CCGR4);
	if (is_mx6dqp()) {
		writel(0x0F0000F3, &ccm->CCGR5);
	} else {
		writel(0x0F0000C3, &ccm->CCGR5);
	}
	writel(0x000003FF, &ccm->CCGR6);
}

static void gpr_init(void)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;

	/* enable AXI cache for VDOA/VPU/IPU */
	writel(0xF00000CF, &iomux->gpr[4]);
	if (is_mx6dqp()) {
		/* set IPU AXI-id1 Qos=0x1 AXI-id0/2/3 Qos=0x7	*/
		writel(0x77177717, &iomux->gpr[6]);
		writel(0x77177717, &iomux->gpr[7]);
	} else {
		/* set IPU AXI-id0 Qos=0xf(bypass) AXI-id1 Qos=0x7 */
		writel(0x007F007F, &iomux->gpr[6]);
		writel(0x007F007F, &iomux->gpr[7]);
	}
}

static int power_init_pmic_sw2(void)
{
	if (!is_som_solo() && !is_dart_board()) {
		unsigned char reg;

		i2c_set_bus_num(PMIC_I2C_BUS);

		if (i2c_probe(CONFIG_POWER_PFUZE100_I2C_ADDR))
			return -1;

		/* Set SW2 to 3.2V */
		if (i2c_read(CONFIG_POWER_PFUZE100_I2C_ADDR, PFUZE100_SW2VOL, 1, &reg, 1))
			return -1;

		reg &= ~0x7f;
		reg |= 0x70; /* SW2x 3.2V */

		if (i2c_write(CONFIG_POWER_PFUZE100_I2C_ADDR, PFUZE100_SW2VOL, 1, &reg, 1))
			return -1;
	}

	return 0;
}

static void setup_iomux_var_per_vcc_en(void)
{
	SETUP_IOMUX_PAD(PAD_EIM_D31__GPIO3_IO31 | MUX_PAD_CTRL(PER_VCC_EN_PAD_CTRL));
	gpio_request(IMX_GPIO_NR(3, 31), "3.3V Enable");
	gpio_direction_output(IMX_GPIO_NR(3, 31), 1);
}

static void setup_iomux_audiocodec(void)
{
	SETUP_IOMUX_PAD(PAD_GPIO_19__GPIO4_IO05 | MUX_PAD_CTRL(NO_PAD_CTRL));
	gpio_request(IMX_GPIO_NR(4, 5), "Audio Codec Reset");
	gpio_direction_output(IMX_GPIO_NR(4, 5), 1);
}

static void audiocodec_reset(int rst)
{
	gpio_set_value(IMX_GPIO_NR(4, 5), !rst);
}

/*
 * Bugfix: Fix Freescale wrong processor documentation.
 */
static void spl_mx6qd_dram_setup_iomux_check_reset(void)
{
	if (is_mx6dq() || is_mx6dqp()) {
		volatile struct mx6dq_iomux_ddr_regs *mx6dq_ddr_iomux;

		mx6dq_ddr_iomux = (struct mx6dq_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;

		if (mx6dq_ddr_iomux->dram_reset == (u32)0x000C0030)
			mx6dq_ddr_iomux->dram_reset = (u32)0x00000030;
	}
}

static void spl_dram_init(void)
{
	if (is_dart_board())
		var_eeprom_v2_dram_init();
	else
		if (var_eeprom_v1_dram_init())
			var_legacy_dram_init(is_som_solo());

	spl_mx6qd_dram_setup_iomux_check_reset();
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();
	gpr_init();

	setup_iomux_var_per_vcc_en();
	setup_iomux_audiocodec();
	audiocodec_reset(1);

	/* setup GP timer */
	timer_init();

	/* iomux and setup of i2c */
	board_early_init_f();

	mdelay(150);

#ifdef LOW_POWER_MODE_ENABLE
	power_init_pmic_sw2();
#endif

	mdelay(180);

	audiocodec_reset(0);

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}
#endif /* CONFIG_SPL_BUILD */
