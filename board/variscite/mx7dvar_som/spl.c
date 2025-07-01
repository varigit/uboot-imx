// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 */

#include <init.h>
#include <spl.h>
#include <fsl_esdhc_imx.h>
#include <linux/delay.h>
#include <asm/sections.h>
#include <asm/arch/clock.h>
#include <asm/arch/mx7-pins.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/sys_proto.h>

#include "mx7dvar_eeprom.h"

/* Initialized to dummy value to not be in the BSS section */
struct mx7d_var_eeprom e = {1};

#define CHECK_BITS_SET	0x80000000
#define END_OF_TABLE	0x00000000

#define I2C_PAD_CTRL    (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

	#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
		PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for EEPROM */
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

static const iomux_v3_cfg_t usdhc1_pads[] = {
	MX7D_PAD_SD1_CLK__SD1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_CMD__SD1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_DATA3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),

	MX7D_PAD_SD1_CD_B__GPIO5_IO0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD1_RESET_B__GPIO5_IO2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static const iomux_v3_cfg_t usdhc3_emmc_pads[] = {
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
		(((0x21) << CCM_ANALOG_PLL_DDR_DIV_SELECT_SHIFT) &
		CCM_ANALOG_PLL_DDR_DIV_SELECT_MASK),
		&ccm_anatop->pll_ddr);

	writel(0x0, &ccm_anatop->pll_ddr_num);

	writel(CCM_ANALOG_PLL_DDR_TEST_DIV_SELECT_MASK |
		CCM_ANALOG_PLL_DDR_ENABLE_CLK_MASK |
		CCM_ANALOG_PLL_DDR_DIV2_ENABLE_CLK_MASK |
		(((0x21) << CCM_ANALOG_PLL_DDR_DIV_SELECT_SHIFT) &
		CCM_ANALOG_PLL_DDR_DIV_SELECT_MASK),
		&ccm_anatop->pll_ddr);

	check_bits_set((u32)&ccm_anatop->pll_ddr, CCM_ANALOG_PLL_DDR_LOCK_MASK);

	writel(0x1, &ccm_reg->root[DRAM_CLK_ROOT].target_root);
}

void spl_board_init(void)
{
	struct mx7d_var_eeprom *ep = VAR_EEPROM_DATA;

	/* Copy EEPROM contents to DRAM */
	memcpy(ep, &e, sizeof(*ep));
}

static void spl_dram_init(void)
{
	int is_eeprom_valid = !(mx7d_var_eeprom_read_header(&e));

	/*
	 * Since the i.MX7D DDR controller doesn't
	 * support real calibration like the i.MX6,
	 * set the DDR freq. to 400MHz to be extra safe
	 */
	set_ddr_freq_to_400mhz();

	if (is_eeprom_valid) {
		mx7d_var_eeprom_print_production_info(&e);

		ddr_init(e.dcd_table, ARRAY_SIZE(e.dcd_table));
	} else {
		printf("\nUsing default DDR configuration");
		mx7d_var_eeprom_print_legacy_production_info(&e);

		ddr_init(default_dcd_table, ARRAY_SIZE(default_dcd_table));
	}
}

static void setup_local_i2c(void)
{
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
}

int board_mmc_init(struct bd_info *bis)
{
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
		imx_iomux_v3_setup_multiple_pads(usdhc1_pads,
						 ARRAY_SIZE(usdhc1_pads));
		usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
		usdhc_cfg[0].max_bus_width = 4;
		break;
	case SD3_BOOT:
	case MMC3_BOOT:
		puts("mmc2 (eMMC)");
		imx_iomux_v3_setup_multiple_pads(usdhc3_emmc_pads,
						 ARRAY_SIZE(usdhc3_emmc_pads));
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
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	/* setup GP timer */
	timer_init();

	/* uart iomux */
	board_early_init_f();

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* Setup i2c bus 0 */
	setup_local_i2c();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}
