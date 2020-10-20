/*
 * Copyright 2018-2019 NXP
 * Copyright 2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <cpu_func.h>
#include <hang.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <power/pmic.h>

#include <power/pca9450.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <asm/arch/ddr.h>

#include "../common/imx8_eeprom.h"
#include "imx8mp_var_dart.h"

DECLARE_GLOBAL_DATA_PTR;

static struct var_eeprom eeprom = {0};

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
#ifdef CONFIG_SPL_BOOTROM_SUPPORT
	return BOOT_DEVICE_BOOTROM;
#else
	switch (boot_dev_spl) {
	case SD1_BOOT:
	case MMC1_BOOT:
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD3_BOOT:
	case MMC3_BOOT:
		return BOOT_DEVICE_MMC2;
	case QSPI_BOOT:
		return BOOT_DEVICE_NOR;
	case NAND_BOOT:
		return BOOT_DEVICE_NAND;
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	default:
		return BOOT_DEVICE_NONE;
	}
#endif
}

static void spl_dram_init(void)
{
	/* EEPROM initialization */
	var_eeprom_read_header(&eeprom);
	var_eeprom_adjust_dram(&eeprom, &dram_timing);
	ddr_init(&dram_timing);
}

#define I2C_PAD_CTRL (PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
struct i2c_pads_info i2c_pads_dart = {
	.scl = {
		.i2c_mode = MX8MP_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX8MP_PAD_I2C1_SCL__GPIO5_IO14 | PC,
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX8MP_PAD_I2C1_SDA__GPIO5_IO15 | PC,
		.gp = IMX_GPIO_NR(5, 15),
	},
};

struct i2c_pads_info i2c_pads_som = {
	.scl = {
		.i2c_mode = MX8MP_PAD_SD1_DATA4__I2C1_SCL | PC,
		.gpio_mode = MX8MP_PAD_SD1_DATA4__GPIO2_IO06 | PC,
		.gp = IMX_GPIO_NR(2, 6),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_SD1_DATA5__I2C1_SDA | PC,
		.gpio_mode = MX8MP_PAD_SD1_DATA5__GPIO2_IO07 | PC,
		.gp = IMX_GPIO_NR(2, 7),
	},
};

#define USDHC2_RST_GPIO_DART IMX_GPIO_NR(2, 19)
#define USDHC2_RST_GPIO_SOM  IMX_GPIO_NR(4, 22)

#define USDHC_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE |PAD_CTL_PE | \
			 PAD_CTL_FSEL2)
#define USDHC_GPIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_DSE1)

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX8MP_PAD_NAND_WE_B__USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_WP_B__USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA04__USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA05__USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA06__USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_DATA07__USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_RE_B__USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_CE2_B__USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_CE3_B__USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_NAND_CLE__USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	MX8MP_PAD_SD2_CLK__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_CMD__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA0__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA1__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA2__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX8MP_PAD_SD2_DATA3__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_rst_pads_dart[] = {
	MX8MP_PAD_SD2_RESET_B__GPIO2_IO19 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_rst_pads_som[] = {
	MX8MP_PAD_SAI2_RXC__GPIO4_IO22 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR, 0, 8},
};

int board_mmc_init(bd_t *bis)
{
	int i, ret, board_id, rst_gpio;

	board_id = var_detect_board_id();

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC1
	 * mmc1                    USDHC2
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			init_clk_usdhc(1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc2_pads,
					ARRAY_SIZE(usdhc2_pads));

			if (board_id == BOARD_ID_DART) {
				rst_gpio = USDHC2_RST_GPIO_DART;
				imx_iomux_v3_setup_multiple_pads(
					usdhc2_rst_pads_dart,
					ARRAY_SIZE(usdhc2_rst_pads_dart));
			}
			else {
				rst_gpio = USDHC2_RST_GPIO_SOM;
				imx_iomux_v3_setup_multiple_pads(
					usdhc2_rst_pads_som,
					ARRAY_SIZE(usdhc2_rst_pads_som));
			}
			gpio_request(rst_gpio, "usdhc2_reset");
			gpio_direction_output(rst_gpio, 0);
			udelay(500);
			gpio_direction_output(rst_gpio, 1);
			break;
		case 1:
			init_clk_usdhc(2);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC3_BASE_ADDR:
		ret = 1;
		break;
	case USDHC2_BASE_ADDR:
		ret = 1;
		return ret;
	}

	return 1;
}

#ifdef CONFIG_POWER
#define PMIC_I2C_BUS	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;

	ret = power_pca9450b_init(PMIC_I2C_BUS);
	if (ret)
		printf("power init failed");
	p = pmic_get("PCA9450");
	pmic_probe(p);

	/* BUCKxOUT_DVS0/1 control BUCK123 output */
	pmic_reg_write(p, PCA9450_BUCK123_DVS, 0x29);

	/*
	 * increase VDD_SOC to typical value 0.95V before first
	 * DRAM access, set DVS1 to 0.85v for suspend.
	 * Enable DVS control through PMIC_STBY_REQ and
	 * set B1_ENMODE=1 (ON by PMIC_ON_REQ=H)
	 */
	pmic_reg_write(p, PCA9450_BUCK1OUT_DVS0, 0x1C);
	pmic_reg_write(p, PCA9450_BUCK1OUT_DVS1, 0x14);
	pmic_reg_write(p, PCA9450_BUCK1CTRL, 0x59);

	/* Kernel uses OD/OD freq for SOC */
	/* To avoid timing risk from SOC to ARM,increase VDD_ARM to OD voltage 0.95v */
	pmic_reg_write(p, PCA9450_BUCK2OUT_DVS0, 0x1C);

	/* set WDOG_B_CFG to cold reset */
	pmic_reg_write(p, PCA9450_RESET_CTRL, 0xA1);

	/* Set LDO4 voltage to 1.8V */
	pmic_reg_write(p, PCA9450_LDO4CTRL, 0xCA);

	/* Enable I2C level translator */
	pmic_reg_write(p, PCA9450_CONFIG2, 0x03);

	return 0;
}
#endif

void spl_board_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;

	puts("Normal Boot\n");

	/* Copy EEPROM contents to DRAM */
	memcpy(ep, &eeprom, sizeof(*ep));
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	int id = var_detect_board_id();

	if ((id == BOARD_ID_DART) && !strcmp(name, "imx8mp-var-dart"))
		return 0;
	else if ((id == BOARD_ID_SOM) && !strcmp(name, "imx8mp-var-som")) {
		return 0;
	}
	else
		return -1;
}
#endif

void board_init_f(ulong dummy)
{
	int ret, board_id;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	board_early_init_f();

	timer_init();

	preloader_console_init();

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	board_id = var_detect_board_id();

	/* I2C Bus 0 initialization */
	if (board_id == BOARD_ID_DART)
		setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pads_dart);
	else
		setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pads_som);

	/* PMIC initialization */
	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	puts("resetting ...\n");

	reset_cpu(WDOG1_BASE_ADDR);

	return 0;
}
