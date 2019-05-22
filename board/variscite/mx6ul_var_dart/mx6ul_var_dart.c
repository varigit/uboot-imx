/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Copyright (C) 2015-2019 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <linux/sizes.h>
#include <miiphy.h>
#include <mmc.h>
#include <netdev.h>
#include <splash.h>
#include <usb.h>
#include <usb/ehci-ci.h>

#ifdef CONFIG_VIDEO_MXS
#include <asm/mach-imx/video.h>
#include "../drivers/video/mxcfb.h"
#endif

#include "mx6var_eeprom_v2.h"

DECLARE_GLOBAL_DATA_PTR;

#define DDR0_CS0_END 0x021b0040

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |	\
	PAD_CTL_SPEED_HIGH   |					\
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST)

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

#define MDIO_PAD_CTRL  (PAD_CTL_PUS_100K_UP | PAD_CTL_PUE |	\
	PAD_CTL_DSE_48ohm   | PAD_CTL_SRE_FAST | PAD_CTL_ODE)

#define ENET_CLK_PAD_CTRL  (PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)

#define ENET_RX_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_SPEED_HIGH   | PAD_CTL_SRE_FAST)

#define LCD_PAD_CTRL    (PAD_CTL_HYS | PAD_CTL_PUS_100K_UP | PAD_CTL_PUE | \
	PAD_CTL_PKE | PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm)

#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)


#ifdef CONFIG_SYS_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)

static struct i2c_pads_info i2c_pad_info1[] = {
	{
		/* DART-6UL */
		.scl = {
			.i2c_mode  = MX6_PAD_UART4_TX_DATA__I2C1_SCL | PC,
			.gpio_mode = MX6_PAD_UART4_TX_DATA__GPIO1_IO28 | PC,
			.gp = IMX_GPIO_NR(1, 28),
		},
		.sda = {
			.i2c_mode  = MX6_PAD_UART4_RX_DATA__I2C1_SDA | PC,
			.gpio_mode = MX6_PAD_UART4_RX_DATA__GPIO1_IO29 | PC,
			.gp = IMX_GPIO_NR(1, 29),
		},
	},
	{
		/* VAR-SOM-6UL */
		.scl = {
			.i2c_mode  = MX6_PAD_CSI_PIXCLK__I2C1_SCL | PC,
			.gpio_mode = MX6_PAD_CSI_PIXCLK__GPIO4_IO18 | PC,
			.gp = IMX_GPIO_NR(4, 18),
		},
		.sda = {
			.i2c_mode  = MX6_PAD_CSI_MCLK__I2C1_SDA | PC,
			.gpio_mode = MX6_PAD_CSI_MCLK__GPIO4_IO17 | PC,
			.gp = IMX_GPIO_NR(4, 17),
		},
	},
};

static struct i2c_pads_info i2c_pad_info2[] = {
	{
		/* DART-6UL */
		.scl = {
			.i2c_mode  = MX6_PAD_UART5_TX_DATA__I2C2_SCL | PC,
			.gpio_mode = MX6_PAD_UART5_TX_DATA__GPIO1_IO30 | PC,
			.gp = IMX_GPIO_NR(1, 30),
		},
		.sda = {
			.i2c_mode  = MX6_PAD_UART5_RX_DATA__I2C2_SDA | PC,
			.gpio_mode = MX6_PAD_UART5_RX_DATA__GPIO1_IO31 | PC,
			.gp = IMX_GPIO_NR(1, 31),
		},
	},
	{
		/* VAR-SOM-6UL */
		.scl = {
			.i2c_mode  = MX6_PAD_CSI_HSYNC__I2C2_SCL | PC,
			.gpio_mode = MX6_PAD_CSI_HSYNC__GPIO4_IO20 | PC,
			.gp = IMX_GPIO_NR(4, 20),
		},
		.sda = {
			.i2c_mode  = MX6_PAD_CSI_VSYNC__I2C2_SDA | PC,
			.gpio_mode = MX6_PAD_CSI_VSYNC__GPIO4_IO19 | PC,
			.gp = IMX_GPIO_NR(4, 19),
		},
	},
};
#endif

/*
 * Returns true iff the SOM is DART-6UL
 */
static inline bool is_dart_6ul(void)
{
	static int is_dart = -1;

	if (is_dart == -1) {
		imx_iomux_v3_setup_pad(MX6_PAD_NAND_CE0_B__GPIO4_IO13 | MUX_PAD_CTRL(PAD_CTL_PUS_100K_UP));

		gpio_request(IMX_GPIO_NR(4, 13), "SOM ID");
		gpio_direction_input(IMX_GPIO_NR(4, 13));
		is_dart = (gpio_get_value(IMX_GPIO_NR(4, 13)) != 0);

#ifdef CONFIG_NAND_MXS
		imx_iomux_v3_setup_pad(MX6_PAD_NAND_CE0_B__RAWNAND_CE0_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2));
#endif
	}

	return is_dart;
}

/*
 * Returns true iff the SOM is VAR-SOM-6UL
 */
static inline bool is_var_som_6ul(void)
{
	return !is_dart_6ul();
}

enum current_board {
	DART_6UL,
	VAR_SOM_6UL,
};

static enum current_board get_board_indx(void)
{
	if (is_dart_6ul())
		return DART_6UL;
	if (is_var_som_6ul())
		return VAR_SOM_6UL;

	printf("Error identifying board!\n");
	hang();
}

#ifdef CONFIG_SYS_I2C
static int var_eeprom_get_ram_size(void)
{
	u32 read_eeprom_magic;
	u8  read_ram_size;

	i2c_set_bus_num(VAR_DART_EEPROM_I2C_BUS);
	if (i2c_probe(VAR_DART_EEPROM_I2C_ADDR)) {
		eeprom_v2_debug("\nError: Couldn't find EEPROM device\n");
		return -1;
	}

	if ((i2c_read(VAR_DART_EEPROM_I2C_ADDR,
			offsetof(struct var_eeprom_v2_cfg, variscite_magic),
			1,
			(u8 *) &read_eeprom_magic,
			sizeof(read_eeprom_magic))) ||
	   (i2c_read(VAR_DART_EEPROM_I2C_ADDR + 1,
			offsetof(struct var_eeprom_v2_cfg, dram_size) & 0xff,
			1,
			(u8 *) &read_ram_size,
			sizeof(read_ram_size))))
	{
		eeprom_v2_debug("\nError reading data from EEPROM\n");
		return -1;
	}

	if (VARISCITE_MAGIC_V2 != read_eeprom_magic) {
		eeprom_v2_debug("\nError: Data on EEPROM is invalid\n");
		return -1;
	}
	return (read_ram_size * SZ_128M);
}
#endif

int dram_init(void)
{
#ifdef CONFIG_SYS_I2C
	int eeprom_ram_size = var_eeprom_get_ram_size();

	if (eeprom_ram_size > SZ_512M)
		gd->ram_size = get_ram_size((void *)PHYS_SDRAM, eeprom_ram_size);
	else
#endif
	{
		unsigned int volatile * const port1 = (unsigned int *) PHYS_SDRAM;
		unsigned int volatile * port2;
		unsigned int volatile * ddr_cs0_end = (unsigned int *) DDR0_CS0_END;

		/* Set the sdram_size to the actually configured one */
		unsigned int sdram_size = ((*ddr_cs0_end) - 63) * 32;
		do {
			port2 = (unsigned int volatile *) (PHYS_SDRAM + ((sdram_size * 1024 * 1024) / 2));

			*port2 = 0;		/* write zero to start of second half of memory. */
			*port1 = 0x3f3f3f3f;	/* write pattern to start of memory. */

			if ((*port2 == 0x3f3f3f3f) && (sdram_size > 128))
				sdram_size = sdram_size / 2;	/* Next step devide size by half */
			else
				if (*port2 == 0) break;		/* Done actual size found. */

		} while (sdram_size > 128);

		gd->ram_size = (sdram_size * 1024 * 1024);
	}

	return 0;
}

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc1_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

#ifndef CONFIG_NAND_MXS
static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX6_PAD_NAND_RE_B__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_WE_B__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA00__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA01__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA02__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA03__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA04__USDHC2_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA05__USDHC2_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA06__USDHC2_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NAND_DATA07__USDHC2_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};
#else
static iomux_v3_cfg_t const nand_pads[] = {
	MX6_PAD_NAND_DATA00__RAWNAND_DATA00 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA01__RAWNAND_DATA01 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA02__RAWNAND_DATA02 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA03__RAWNAND_DATA03 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA04__RAWNAND_DATA04 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA05__RAWNAND_DATA05 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA06__RAWNAND_DATA06 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DATA07__RAWNAND_DATA07 | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_CLE__RAWNAND_CLE | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_ALE__RAWNAND_ALE | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_CE0_B__RAWNAND_CE0_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_RE_B__RAWNAND_RE_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_WE_B__RAWNAND_WE_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_WP_B__RAWNAND_WP_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_READY_B__RAWNAND_READY_B | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
	MX6_PAD_NAND_DQS__RAWNAND_DQS | MUX_PAD_CTRL(GPMI_PAD_CTRL2),
};

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* config gpmi nand iomux */
	imx_iomux_v3_setup_multiple_pads(nand_pads, ARRAY_SIZE(nand_pads));

	clrbits_le32(&mxc_ccm->CCGR4,
		     MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_MASK);

	/*
	 * config gpmi and bch clock to 100 MHz
	 * bch/gpmi select PLL2 PFD2 400M
	 * 100M = 400M / 4
	 */
	clrbits_le32(&mxc_ccm->cscmr1,
		     MXC_CCM_CSCMR1_BCH_CLK_SEL |
		     MXC_CCM_CSCMR1_GPMI_CLK_SEL);
	clrsetbits_le32(&mxc_ccm->cscdr1,
			MXC_CCM_CSCDR1_BCH_PODF_MASK |
			MXC_CCM_CSCDR1_GPMI_PODF_MASK,
			(3 << MXC_CCM_CSCDR1_BCH_PODF_OFFSET) |
			(3 << MXC_CCM_CSCDR1_GPMI_PODF_OFFSET));

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
		     MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_MASK);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

static struct fsl_esdhc_cfg usdhc_cfg[2];

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_ENV_SUPPORT)
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
#endif

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

int board_mmc_init(bd_t *bis)
{
#ifndef CONFIG_SPL_BUILD
	int i;
	int err;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    USDHC1 (SD)
	 * mmc1                    USDHC2 (eMMC)
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			imx_iomux_v3_setup_multiple_pads(
					usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
			usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
			usdhc_cfg[0].max_bus_width = 4;
			break;
#ifndef CONFIG_NAND_MXS
		case 1:
			imx_iomux_v3_setup_multiple_pads(
					usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			usdhc_cfg[1].esdhc_base = USDHC2_BASE_ADDR;
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			usdhc_cfg[1].max_bus_width = 8;
			break;
#endif
		default:
			printf("Warning: you configured more USDHC controllers (%d) than supported by the board\n", i + 1);
			return 0;
		}

		err = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (err)
			printf("Error: failed to initialize mmc dev %d\n", i);
	}
	return 0;
#else
	int devno = mmc_get_env_dev();
	puts("MMC Boot Device: ");
	switch (devno) {
	case 0:
		puts("mmc0 (SD)\n");
		imx_iomux_v3_setup_multiple_pads(
				usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
		usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
		usdhc_cfg[0].max_bus_width = 4;
		break;
#ifndef CONFIG_NAND_MXS
	case 1:
		puts("mmc1 (eMMC)\n");
		imx_iomux_v3_setup_multiple_pads(
				usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
		usdhc_cfg[0].esdhc_base = USDHC2_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
		usdhc_cfg[0].max_bus_width = 8;
		break;
#endif
	default:
		printf("Error: Unsupported mmc dev num %d\n", devno);
		return -1;
	}

	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
#endif
}

#ifndef CONFIG_SPL_BUILD
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

static void print_emmc_size(void)
{
	struct mmc *mmc;
	int err;

	mmc = find_mmc_device(1);
	err = !mmc;
	if (!err)
		err = mmc_init(mmc);

	if (err)
		return;

	puts("eMMC:  ");
	print_size(mmc->capacity, "\n");
}

#ifdef CONFIG_VIDEO_MXS
static iomux_v3_cfg_t const lcd_pads[] = {
	MX6_PAD_LCD_CLK__LCDIF_CLK | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_ENABLE__LCDIF_ENABLE | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA02__LCDIF_DATA02 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA03__LCDIF_DATA03 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA04__LCDIF_DATA04 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA05__LCDIF_DATA05 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA06__LCDIF_DATA06 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA07__LCDIF_DATA07 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA10__LCDIF_DATA10 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA11__LCDIF_DATA11 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA12__LCDIF_DATA12 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA13__LCDIF_DATA13 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA14__LCDIF_DATA14 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA15__LCDIF_DATA15 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA18__LCDIF_DATA18 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA19__LCDIF_DATA19 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA20__LCDIF_DATA20 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA21__LCDIF_DATA21 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA22__LCDIF_DATA22 | MUX_PAD_CTRL(LCD_PAD_CTRL),
	MX6_PAD_LCD_DATA23__LCDIF_DATA23 | MUX_PAD_CTRL(LCD_PAD_CTRL),
};

static iomux_v3_cfg_t const pwm_pads[][1*2] = {
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

static int backlight_gpio[] = {
	/* DART-6UL */
	IMX_GPIO_NR(3, 5),
	/* VAR-SOM-6UL */
	IMX_GPIO_NR(1, 5)
};

void do_enable_parallel_lcd(struct display_info_t const *dev)
{
	if (!is_cpu_type(MXC_CPU_MX6ULZ)) {
		int board = get_board_indx();

		enable_lcdif_clock(dev->bus, 1);

		imx_iomux_v3_setup_multiple_pads(lcd_pads, ARRAY_SIZE(lcd_pads));

		imx_iomux_v3_setup_multiple_pads(pwm_pads[board], ARRAY_SIZE(pwm_pads[board]));

		/* Set Brightness to high */
		gpio_request(backlight_gpio[board], "backlight");
		gpio_direction_output(backlight_gpio[board], 1);
	}
}

#define MHZ2PS(f)       (1000000/(f))

struct display_info_t const displays[] = {{
	.bus = MX6UL_LCDIF1_BASE_ADDR,
	.addr = 0,
	.pixfmt = 24,
	.detect = NULL,
	.enable	= do_enable_parallel_lcd,
	.mode	= {
		.name           = "VAR-WVGA-LCD",
		.xres           = 800,
		.yres           = 480,
		.pixclock       = MHZ2PS(30),
		.left_margin    = 40,
		.right_margin   = 40,
		.upper_margin   = 29,
		.lower_margin   = 13,
		.hsync_len      = 48,
		.vsync_len      = 3,
		.sync           = FB_SYNC_CLK_LAT_FALL,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
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
	int ret = 0;

	if (!is_cpu_type(MXC_CPU_MX6ULZ)) {
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
	}

	return ret;
}
#endif /* CONFIG_SPLASH_SCREEN */
#endif /* CONFIG_SPL_BUILD */

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB

#define USB_OTHERREGS_OFFSET	0x800
#define UCTRL_PWR_POL		(1 << 9)

static iomux_v3_cfg_t const usb_otg_pads[] = {
	MX6_PAD_UART3_TX_DATA__ANATOP_OTG1_ID | MUX_PAD_CTRL(NO_PAD_CTRL),
};

/* At default the 3v3 enables the MIC2026 for VBUS power */
static void setup_usb(void)
{
	if (is_var_som_6ul())
		imx_iomux_v3_setup_multiple_pads(usb_otg_pads,
						 ARRAY_SIZE(usb_otg_pads));
}

int board_usb_phy_mode(int port)
{
	if (port == 1) {
		return USB_INIT_HOST;
	} else {
		if (is_dart_6ul()) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_ENV_SUPPORT)
			if (env_check("usbmode", "host"))
				return USB_INIT_HOST;
			else
#endif
				return USB_INIT_DEVICE;
		} else {
			return usb_phy_mode(port);
		}
	}
}

int board_ehci_hcd_init(int port)
{
	u32 *usbnc_usb_ctrl;

	if (port > 1)
		return -EINVAL;

	usbnc_usb_ctrl = (u32 *)(USB_BASE_ADDR + USB_OTHERREGS_OFFSET +
				 port * 4);

	/* Set Power polarity */
	setbits_le32(usbnc_usb_ctrl, UCTRL_PWR_POL);

	return 0;
}
#endif
#endif

#ifdef CONFIG_FEC_MXC
/*
 * pin conflicts for fec1 and fec2, GPIO1_IO06 and GPIO1_IO07 can only
 * be used for ENET1 or ENET2, cannot be used for both.
 */
static iomux_v3_cfg_t const fec1_pads[] = {
	MX6_PAD_GPIO1_IO06__ENET1_MDIO | MUX_PAD_CTRL(MDIO_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET1_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA0__ENET1_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_DATA1__ENET1_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_EN__ENET1_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_TX_CLK__ENET1_REF_CLK1 | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA0__ENET1_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_DATA1__ENET1_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_ER__ENET1_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET1_RX_EN__ENET1_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static iomux_v3_cfg_t const fec2_pads[] = {
	MX6_PAD_GPIO1_IO06__ENET2_MDIO | MUX_PAD_CTRL(MDIO_PAD_CTRL),
	MX6_PAD_GPIO1_IO07__ENET2_MDC | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET2_TX_DATA0__ENET2_TDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_DATA1__ENET2_TDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_TX_CLK__ENET2_REF_CLK2 | MUX_PAD_CTRL(ENET_CLK_PAD_CTRL),
	MX6_PAD_ENET2_TX_EN__ENET2_TX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),

	MX6_PAD_ENET2_RX_DATA0__ENET2_RDATA00 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_DATA1__ENET2_RDATA01 | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_EN__ENET2_RX_EN | MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET2_RX_ER__ENET2_RX_ER | MUX_PAD_CTRL(ENET_PAD_CTRL),
};

static void setup_iomux_fec(int fec_id)
{
	if (fec_id == 0)
		imx_iomux_v3_setup_multiple_pads(fec1_pads,
						 ARRAY_SIZE(fec1_pads));
	else
		imx_iomux_v3_setup_multiple_pads(fec2_pads,
						 ARRAY_SIZE(fec2_pads));
}

int board_eth_init(bd_t *bis)
{
	int ret = 0;
	if (!is_cpu_type(MXC_CPU_MX6ULZ)) {
		setup_iomux_fec(CONFIG_FEC_ENET_DEV);

		ret = fecmxc_initialize_multi(bis, CONFIG_FEC_ENET_DEV,
				CONFIG_FEC_MXC_PHYADDR, IMX_FEC_BASE);
	}

#if defined(CONFIG_CI_UDC) && defined(CONFIG_USB_ETHER)
	/* USB Ethernet Gadget */
	usb_eth_initialize(bis);
#endif

	return ret;
}

static int setup_fec(int fec_id)
{
	if (!is_cpu_type(MXC_CPU_MX6ULZ)) {
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
	}

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	if (!is_cpu_type(MXC_CPU_MX6ULZ)) {
		phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8190);

		if (phydev->drv->config)
			phydev->drv->config(phydev);
	}

	return 0;
}
#endif

static void setup_local_i2c(void)
{
	int board = get_board_indx();
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1[board]);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2[board]);
}

int board_early_init_f(void)
{
#ifdef CONFIG_VIDEO_MXS
	if (is_cpu_type(MXC_CPU_MX6ULZ))
		display_count = 0;
#endif
	setup_iomux_uart();

#ifdef CONFIG_SYS_I2C_MXC
	setup_local_i2c();
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

#ifdef CONFIG_USB_EHCI_MX6
#ifndef CONFIG_DM_USB
	setup_usb();
#endif
#endif

#ifdef CONFIG_NAND_MXS
	setup_gpmi_nand();
#endif

	return 0;
}

#ifndef CONFIG_SPL_BUILD
#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"sd1", MAKE_CFGVAL(0x42, 0x20, 0x00, 0x00)},
	{"sd2", MAKE_CFGVAL(0x40, 0x28, 0x00, 0x00)},
	{NULL,	 0},
};
#endif

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	struct var_eeprom_v2_cfg var_eeprom_v2_cfg = {0};

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_init();
#endif

	if (var_eeprom_v2_read_struct(&var_eeprom_v2_cfg))
		puts("Warning: Can't read SOM configuration from EEPROM\n");

	if ((var_eeprom_v2_cfg.som_info & 0x3) == 0x2)
		print_emmc_size();

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (is_dart_6ul())
		env_set("board_name", "DART-6UL");
	else
		env_set("board_name", "VAR-SOM-6UL");

	if (is_cpu_type(MXC_CPU_MX6ULL))
		env_set("soc_type", "imx6ull");
	else if (is_cpu_type(MXC_CPU_MX6ULZ))
		env_set("soc_type", "imx6ulz");
	else
		env_set("soc_type", "imx6ul");

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

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

	if (var_eeprom_v2_cfg.som_info & 0x4)
		env_set("wifi", "yes");
	else
		env_set("wifi", "no");

	switch ((var_eeprom_v2_cfg.som_info >> 3) & 0x3) {
	case 0x0:
		env_set("som_rev", "2.4G");
		break;
	case 0x1:
		env_set("som_rev", "5G");
		break;
	default:
		env_set("som_rev", "unknown");
		break;
	}

	switch (var_eeprom_v2_cfg.som_info & 0x3) {
	case 0x00:
		env_set("som_storage", "none");
		break;
	case 0x01:
		env_set("som_storage", "nand");
		break;
	case 0x02:
		env_set("som_storage", "emmc");
		break;
	default:
		env_set("som_storage", "unknown");
		break;
	}
#endif

	return 0;
}

int checkboard(void)
{
	puts("Board: Variscite ");
	if (is_dart_6ul())
		puts("DART-6UL\n");
	else
		puts("VAR-SOM-6UL\n");

	return 0;
}
#endif

#ifdef CONFIG_SPL_BUILD
#include <asm/arch/mx6-ddr.h>
#include <linux/libfdt.h>
#include <spl.h>

static struct mx6ul_iomux_grp_regs mx6_grp_ioregs = {
	.grp_addds = 0x00000030,
	.grp_ddrmode_ctl = 0x00020000,
	.grp_b0ds = 0x00000030,
	.grp_ctlds = 0x00000030,
	.grp_b1ds = 0x00000030,
	.grp_ddrpke = 0x00000000,
	.grp_ddrmode = 0x00020000,
	.grp_ddr_type = 0x000c0000,
};

static struct mx6ul_iomux_ddr_regs mx6_ddr_ioregs = {
	.dram_dqm0 = 0x00000030,
	.dram_dqm1 = 0x00000030,
	.dram_ras = 0x00000030,
	.dram_cas = 0x00000030,
	.dram_odt0 = 0x00000030,
	.dram_odt1 = 0x00000030,
	.dram_sdba2 = 0x00000000,
	.dram_sdclk_0 = 0x00000008,
	.dram_sdqs0 = 0x00000038,
	.dram_sdqs1 = 0x00000030,
	.dram_reset = 0x00000030,
};

static struct mx6_mmdc_calibration mx6_mmcd_calib = {
	.p0_mpwldectrl0 = 0x00000000,
	.p0_mpdgctrl0   = 0x414C0158,
	.p0_mprddlctl   = 0x40403A3A,
	.p0_mpwrdlctl   = 0x40405A56,
};

struct mx6_ddr_sysinfo ddr_sysinfo = {
	.dsize = 0,
	.cs_density = 20,
	.ncs = 1,
	.cs1_mirror = 0,
	.rtt_wr = 2,
	.rtt_nom = 1,		/* RTT_Nom = RZQ/2 */
	.walat = 1,		/* Write additional latency */
	.ralat = 5,		/* Read additional latency */
	.mif3_mode = 3,		/* Command prediction working mode */
	.bi_on = 1,		/* Bank interleaving enabled */
	.sde_to_rst = 0x10,	/* 14 cycles, 200us (JEDEC default) */
	.rst_to_cke = 0x23,	/* 33 cycles, 500us (JEDEC default) */
};

static struct mx6_ddr3_cfg mem_ddr = {
	.mem_speed = 800,
	.density = 4,
	.width = 16,
	.banks = 8,
	.rowaddr = 15,
	.coladdr = 10,
	.pagesz = 2,
	.trcd = 1375,
	.trcmin = 4875,
	.trasmin = 3500,
};

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(0xFFFFFFFF, &ccm->CCGR0);
	writel(0xFFFFFFFF, &ccm->CCGR1);
	writel(0xFFFFFFFF, &ccm->CCGR2);
	writel(0xFFFFFFFF, &ccm->CCGR3);
	writel(0xFFFFFFFF, &ccm->CCGR4);
	writel(0xFFFFFFFF, &ccm->CCGR5);
	writel(0xFFFFFFFF, &ccm->CCGR6);
	writel(0xFFFFFFFF, &ccm->CCGR7);
	/* Enable Audio Clock for SOM codec */
	writel(0x01130100, (long *)CCM_CCOSR);
}

static void legacy_dram_init(void)
{
	mx6ul_dram_iocfg(mem_ddr.width, &mx6_ddr_ioregs, &mx6_grp_ioregs);
	mx6_dram_cfg(&ddr_sysinfo, &mx6_mmcd_calib, &mem_ddr);
}

void spl_dram_init(void)
{
	struct var_eeprom_v2_cfg var_eeprom_v2_cfg = {0};

	if (var_eeprom_v2_read_struct(&var_eeprom_v2_cfg)) {
		legacy_dram_init();
		puts("DDR LEGACY configuration\n");
		return;
	}

	var_eeprom_v2_dram_init(&var_eeprom_v2_cfg);

	var_eeprom_v2_print_production_info(&var_eeprom_v2_cfg);
	var_eeprom_v2_print_som_info(&var_eeprom_v2_cfg);
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();

	/* setup GP timer */
	timer_init();

	/* iomux and setup of i2c */
	board_early_init_f();

	mdelay(200);

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
