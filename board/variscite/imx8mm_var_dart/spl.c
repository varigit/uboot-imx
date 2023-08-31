// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2018-2019 NXP
 * Copyright 2019-2020 Variscite Ltd.
 */

#include <common.h>
#include <cpu_func.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <spl.h>
#include <asm/global_data.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <power/pmic.h>
#include <power/bd71837.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/arch/ddr.h>
#include <fsl_sec.h>
#include <linux/delay.h>

#include "../common/imx8_eeprom.h"
#include "imx8mm_var_dart.h"

DECLARE_GLOBAL_DATA_PTR;

static struct var_eeprom eeprom = {0};

extern struct dram_timing_info dram_timing_ddr4, dram_timing_lpddr4;

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
	switch (boot_dev_spl) {
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

	return BOOT_DEVICE_NONE;
}

static void spl_dram_init(void)
{
	int id;

	id = get_board_id();

	if (id == DART_MX8M_MINI) {
		var_eeprom_read_header(&eeprom);
		var_eeprom_adjust_dram(&eeprom, &dram_timing_lpddr4);
		ddr_init(&dram_timing_lpddr4);
	} else if (id == VAR_SOM_MX8M_MINI) {
		var_eeprom_read_header(&eeprom);
		var_eeprom_adjust_dram(&eeprom, &dram_timing_ddr4);
		ddr_init(&dram_timing_ddr4);
	} else {
		printf("Undefined board ID\n");
		return;
	}
}

#define USDHC2_PWR_GPIO_DART	IMX_GPIO_NR(2, 19)
#define USDHC2_PWR_GPIO_SOM	IMX_GPIO_NR(4, 22)

#define USDHC_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE | \
			 PAD_CTL_FSEL2)
#define USDHC_GPIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_DSE1)

static const iomux_v3_cfg_t usdhc3_pads[] = {
	IMX8MM_PAD_NAND_WE_B_USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_WP_B_USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA04_USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA05_USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA06_USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_DATA07_USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_RE_B_USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_CE2_B_USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_CE3_B_USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_NAND_CLE_USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static const iomux_v3_cfg_t usdhc2_pads[] = {
	IMX8MM_PAD_SD2_CLK_USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_CMD_USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA0_USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA1_USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA2_USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MM_PAD_SD2_DATA3_USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static const iomux_v3_cfg_t usdhc2_pwr_pads_dart[] = {
	IMX8MM_PAD_SD2_RESET_B_GPIO2_IO19 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

static const iomux_v3_cfg_t usdhc2_pwr_pads_som[] = {
	IMX8MM_PAD_SAI2_RXC_GPIO4_IO22 | MUX_PAD_CTRL(USDHC_GPIO_PAD_CTRL),
};

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR, 0, 8},
};

int board_mmc_init(struct bd_info *bis)
{
	int i, ret;

	/*
	 * According to the board_mmc_init() the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC2
	 * mmc1                    USDHC3
	 */
	for (i = 0; i < CFG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			init_clk_usdhc(1);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			imx_iomux_v3_setup_multiple_pads(usdhc2_pads,
						ARRAY_SIZE(usdhc2_pads));
			if (get_board_id() == VAR_SOM_MX8M_MINI) {
				imx_iomux_v3_setup_multiple_pads(usdhc2_pwr_pads_som,
						ARRAY_SIZE(usdhc2_pwr_pads_som));
				gpio_request(USDHC2_PWR_GPIO_SOM, "usdhc2_reset");
				gpio_direction_output(USDHC2_PWR_GPIO_SOM, 0);
				udelay(500);
				gpio_direction_output(USDHC2_PWR_GPIO_SOM, 1);
			} else {
				imx_iomux_v3_setup_multiple_pads(usdhc2_pwr_pads_dart,
						ARRAY_SIZE(usdhc2_pwr_pads_dart));
				gpio_request(USDHC2_PWR_GPIO_DART, "usdhc2_reset");
				gpio_direction_output(USDHC2_PWR_GPIO_DART, 0);
				udelay(500);
				gpio_direction_output(USDHC2_PWR_GPIO_DART, 1);
			}
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

#if CONFIG_IS_ENABLED(POWER_LEGACY)
#define PMIC_I2C_BUS	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;

	ret = power_bd71837_init(PMIC_I2C_BUS);
	if (ret)
		printf("power init failed");

	p = pmic_get("BD71837");
	pmic_probe(p);

	/* decrease RESET key long push time from the default 10s to 10ms */
	pmic_reg_write(p, BD718XX_PWRONCONFIG1, 0x0);

	/* unlock the PMIC regs */
	pmic_reg_write(p, BD718XX_REGLOCK, 0x1);

	/* increase VDD_SOC to typical value 0.85v before first DRAM access */
	pmic_reg_write(p, BD718XX_BUCK1_VOLT_RUN, 0x0f);

	/* increase VDD_DRAM to 0.975v for 3Ghz DDR */
	pmic_reg_write(p, BD718XX_1ST_NODVS_BUCK_VOLT, 0x83);

	/* increase NVCC_DRAM_1V2 to 1.2v for DDR4 */
	if (get_board_id() == VAR_SOM_MX8M_MINI)
		pmic_reg_write(p, BD718XX_4TH_NODVS_BUCK_VOLT, 0x28);

	/* enable LDO5 - required to access I2C bus 3 */
	pmic_reg_write(p, BD718XX_LDO5_VOLT, 0xc0);

	/* lock the PMIC regs */
	pmic_reg_write(p, BD718XX_REGLOCK, 0x11);

	return 0;
}
#endif

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)

static const iomux_v3_cfg_t uart1_pads[] = {
	IMX8MM_PAD_UART1_RXD_UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART1_TXD_UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static const iomux_v3_cfg_t uart4_pads[] = {
	IMX8MM_PAD_UART4_RXD_UART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART4_TXD_UART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

extern struct mxc_uart *mxc_base;

int uart_init(void)
{
	int id;

	id = get_board_id();

	if (id == DART_MX8M_MINI) {
		init_uart_clk(0);
		imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
	} else if (id == VAR_SOM_MX8M_MINI) {
		init_uart_clk(3);
		imx_iomux_v3_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
		mxc_base = (struct mxc_uart *)CFG_MXC_UART_BASE_2;
	}

	return 0;
}

void spl_board_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;

if (IS_ENABLED(CONFIG_FSL_CAAM)) {
	if (sec_init())
		printf("\nsec_init failed!\n");
}
#ifndef CONFIG_SPL_USB_SDP_SUPPORT
	/* Serial download mode */
	if (is_usb_boot()) {
		puts("Back to ROM, SDP\n");
		restore_boot_params();
	}
#endif
	puts("Normal Boot\n");

	/* Copy EEPROM contents to DRAM */
	memcpy(ep, &eeprom, sizeof(*ep));
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	int id = get_board_id();

	if ((id == DART_MX8M_MINI) && !strcmp(name, "imx8mm-var-dart-customboard"))
		return 0;
	else if ((id == VAR_SOM_MX8M_MINI) && !strcmp(name, "imx8mm-var-som-symphony"))
		return 0;
	else
		return -1;
}
#endif
#define I2C_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)

static struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = IMX8MM_PAD_I2C1_SCL_I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = IMX8MM_PAD_I2C1_SCL_GPIO5_IO14 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = IMX8MM_PAD_I2C1_SDA_I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = IMX8MM_PAD_I2C1_SDA_GPIO5_IO15 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 15),
	},
};

void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	board_early_init_f();

	timer_init();

	uart_init();

	preloader_console_init();

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	/* I2C Bus 0 initialization */
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);

	/* PMIC initialization */
	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
