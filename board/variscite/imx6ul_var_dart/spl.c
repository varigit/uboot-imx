// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 * Copyright (C) 2019 Parthiban Nallathambi <parthitce@gmail.com>
 */

#include <fsl_esdhc_imx.h>
#include <init.h>
#include <spl.h>
#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/mx6-ddr.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/sections.h>
#include <linux/delay.h>

#include "imx6ul_var_dart-common.h"
#include "mx6var_eeprom_v2.h"

DECLARE_GLOBAL_DATA_PTR;

/* Initialized to dummy value to not be in the BSS section */
struct var_eeprom e = {1};

enum {
	BOARD_TYPE_DART_6UL,
	BOARD_TYPE_VAR_SOM_6UL,
};

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

static const iomux_v3_cfg_t uart1_pads[] = {
	MX6_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

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

void spl_board_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;

	/* Copy EEPROM contents to DRAM */
	memcpy(ep, &e, sizeof(*ep));
}

static void spl_dram_init(void)
{
	if (var_eeprom_read_header(&e)) {
		mx6ul_dram_iocfg(mem_ddr.width, &mx6_ddr_ioregs, &mx6_grp_ioregs);
		mx6_dram_cfg(&ddr_sysinfo, &mx6_mmcd_calib, &mem_ddr);
		puts("DDR LEGACY configuration\n");
		return;
	}

	var_eeprom_dram_init(&e);
}

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_22K_UP  | PAD_CTL_SPEED_LOW |		\
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

static const iomux_v3_cfg_t usdhc1_pads[] = {
	MX6_PAD_SD1_CLK__USDHC1_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__USDHC1_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA0__USDHC1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA1__USDHC1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA2__USDHC1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DATA3__USDHC1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

#if !IS_ENABLED(CONFIG_NAND_MXS)
static const iomux_v3_cfg_t usdhc2_pads[] = {
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
#endif

static struct fsl_esdhc_cfg usdhc_cfg[] = {
	{
		.esdhc_base = USDHC1_BASE_ADDR,
		.max_bus_width = 4,
	},
#if !IS_ENABLED(CONFIG_NAND_MXS)
	{
		.esdhc_base = USDHC2_BASE_ADDR,
		.max_bus_width = 8,
	},
#endif
};

#define I2C_PAD_CTRL    (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE)

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

static void setup_local_i2c(void)
{
	int board = get_board_indx();

	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1[board]);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2[board]);
}

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

int board_mmc_init(struct bd_info *bis)
{
	int devno = mmc_get_env_dev();

	puts("MMC Boot Device: ");
	switch (devno) {
	case 0:
		puts("mmc0 (SD)\n");
		imx_iomux_v3_setup_multiple_pads(usdhc1_pads, ARRAY_SIZE(usdhc1_pads));
		usdhc_cfg[0].esdhc_base = USDHC1_BASE_ADDR;
		usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
		usdhc_cfg[0].max_bus_width = 4;
		break;
#if !IS_ENABLED(CONFIG_NAND_MXS)
	case 1:
		puts("mmc1 (eMMC)\n");
		imx_iomux_v3_setup_multiple_pads(usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
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
}

static int do_board_detect(void)
{
	if (is_dart_6ul())
		gd->board_type = BOARD_TYPE_DART_6UL;
	else if (is_var_som_6ul())
		gd->board_type = BOARD_TYPE_VAR_SOM_6UL;

	return 0;
}

#if IS_ENABLED(CONFIG_SPL_LOAD_FIT)

static int match_name(const char *name, const char *expected_name)
{
	return !strcmp(name, expected_name);
}

static int check_match(const char *name, const char *base, const char *suffix)
{
	char full_name[128];

	snprintf(full_name, sizeof(full_name), "%s%s", base, suffix);
	return match_name(name, full_name);
}

int board_fit_config_name_match(const char *name)
{
	const char *board_prefix = NULL;

	if (gd->board_type == BOARD_TYPE_DART_6UL) {
		if (is_mx6ulz())
			board_prefix = "imx6ulz-var-dart-6ulcustomboard-";
		else if (is_mx6ul())
			board_prefix = "imx6ul-var-dart-6ulcustomboard-";
		else if (is_mx6ull())
			board_prefix = "imx6ull-var-dart-6ulcustomboard-";
	} else if (gd->board_type == BOARD_TYPE_VAR_SOM_6UL) {
		if (!is_symphony()) {
			if (is_mx6ulz())
				board_prefix = "imx6ulz-var-som-concerto-board-";
			else if (is_mx6ul())
				board_prefix = "imx6ul-var-som-concerto-board-";
			else if (is_mx6ull())
				board_prefix = "imx6ull-var-som-concerto-board-";
		} else {
			if (is_mx6ulz())
				board_prefix = "imx6ulz-var-som-symphony-board-";
			else if (is_mx6ul())
				board_prefix = "imx6ul-var-som-symphony-board-";
			else if (is_mx6ull())
				board_prefix = "imx6ull-var-som-symphony-board-";
		}
	}

	if (board_prefix) {
#if IS_ENABLED(CONFIG_NAND_MXS)
		if (check_match(name, board_prefix, "nand-sd-card"))
			return 0;
#else
		if (check_match(name, board_prefix, "emmc-sd-card"))
			return 0;
#endif
	}

	return -1;
}
#endif /* CONFIG_SPL_LOAD_FIT */

/* add an extra delay to make sure that voltages are stable for serial debug output.
 * This is only needed for DART modules when booting from eMMC
 * VAR-SOM is not affected at all.
 *
 * This function is just needed for the complete debug UART to also contain the output
 * from the SPL (which contains the version of the SPL. If you don't care about this,
 * you can omit calling this function and gain some extra boot time.
 */
static void extra_delay_for_stable_uart(void)
{
	int devno;
	if (is_dart_6ul()) {
		devno = mmc_get_env_dev();
		if (devno == 1) {	/* booting from eMMC */
			mdelay(160);	/* 150ms was measured, 10ms extra to be safe */
		}
	}
}

void board_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();

	/* setup GP timer */
	timer_init();

	setup_iomux_uart();

	/* make sure that UART voltages are stable on all boards */
	extra_delay_for_stable_uart();

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* Detect the board variant */
	do_board_detect();

	/*Setup i2c1 */
	setup_local_i2c();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}
