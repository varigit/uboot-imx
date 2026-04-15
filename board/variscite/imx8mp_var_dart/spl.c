// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2018-2019, 2021 NXP
 * Copyright 2020-2025 Variscite Ltd.
 *
 */

#include <common.h>
#include <cpu_func.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/boot_mode.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <power/pmic.h>

#include <power/pca9450.h>
#include <asm/arch/clock.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <asm/arch/ddr.h>
#include <asm/sections.h>

#include "../common/imx8_eeprom.h"
#include "imx8mp_var_dart.h"

DECLARE_GLOBAL_DATA_PTR;

#define GPIO_PAD_CTRL	(PAD_CTL_DSE1 | PAD_CTL_PUE | PAD_CTL_PE  | PAD_CTL_HYS)
#define I2C_PAD_CTRL (PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)

#define LVDS0_BKLT_PWM IMX_GPIO_NR(1, 11)
#define LVDS1_BKLT_PWM IMX_GPIO_NR(5, 5)

static iomux_v3_cfg_t const lvds_bklt_pwm_pads[] = {
	MX8MP_PAD_GPIO1_IO11__GPIO1_IO11 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
	MX8MP_PAD_SPDIF_EXT_CLK__GPIO5_IO05 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static struct i2c_pads_info i2c1_pads_dart = {
	.scl = {
		.i2c_mode = MX8MP_PAD_I2C2_SCL__I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_I2C2_SCL__GPIO5_IO16 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 16),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_I2C2_SDA__I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_I2C2_SDA__GPIO5_IO17 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 17),
	},
};

static struct i2c_pads_info i2c4_pads_som = {
	.scl = {
		.i2c_mode = MX8MP_PAD_I2C4_SCL__I2C4_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_I2C4_SCL__GPIO5_IO20 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 20),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_I2C4_SDA__I2C4_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_I2C4_SDA__GPIO5_IO21 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 21),
	},
};

static struct i2c_pads_info i2c1_pads_smarc = {
	.scl = {
		.i2c_mode = MX8MP_PAD_SD1_DATA6__I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_SD1_DATA6__GPIO2_IO08 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(2, 8),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_SD1_DATA7__I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_SD1_DATA7__GPIO2_IO09 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(2, 9),
	},
};

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)

static iomux_v3_cfg_t const uart_pads_dart[] = {
	MX8MP_PAD_UART1_RXD__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX8MP_PAD_UART1_TXD__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart_pads_som[] = {
	MX8MP_PAD_UART2_RXD__UART2_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX8MP_PAD_UART2_TXD__UART2_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

extern struct mxc_uart *mxc_base;

static struct var_eeprom eeprom = {0};
static struct var_carrier_eeprom carrier_eeprom = {0};

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
	u16 carrier_eeprom_bus;
	int board_id;

	board_id = var_detect_board_id();
	if (board_id == BOARD_ID_DART)
		carrier_eeprom_bus = CARRIER_EEPROM_BUS_DART;
	else if (board_id == BOARD_ID_SOM)
		carrier_eeprom_bus = CARRIER_EEPROM_BUS_SOM;
	else
		carrier_eeprom_bus = CARRIER_EEPROM_BUS_SMARC;

	/* EEPROM initialization */
	var_eeprom_read_header(&eeprom);
	var_carrier_eeprom_read(carrier_eeprom_bus,
				CARRIER_EEPROM_ADDR,
				&carrier_eeprom);
	var_eeprom_adjust_dram(&eeprom, &dram_timing);
	ddr_init(&dram_timing);
}

static void spl_uart_init(void)
{
	int board_id;

	board_id = var_detect_board_id();
	if (board_id == BOARD_ID_DART) {
		imx_iomux_v3_setup_multiple_pads(uart_pads_dart,
			ARRAY_SIZE(uart_pads_dart));
		init_uart_clk(0);
	} else if ((board_id == BOARD_ID_SOM) || (board_id == BOARD_ID_SMARC)) {
		imx_iomux_v3_setup_multiple_pads(uart_pads_som,
			ARRAY_SIZE(uart_pads_som));
		init_uart_clk(1);
		mxc_base = (struct mxc_uart *)UART2_BASE_ADDR;
	}
}

#if CONFIG_IS_ENABLED(POWER_LEGACY)
#define PMIC_I2C_BUS	0
int power_init_board(void)
{
	struct pmic *p;
	int ret;

	ret = power_pca9450_init(PMIC_I2C_BUS, 0x25);
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

	/* Set BUCK5 voltage to 1.85V to fix Ethernet PHY reset */
	if (var_detect_board_id() == BOARD_ID_DART)
		pmic_reg_write(p, PCA9450_BUCK5OUT, 0x32);

	return 0;
}
#endif

void spl_board_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	struct var_carrier_eeprom *carrier_ep = VAR_CARRIER_EEPROM_DATA;

	arch_misc_init();

	/* Set GIC clock to 500Mhz for OD VDD_SOC. Kernel driver does not allow to change it.
	 * Should set the clock after PMIC setting done.
	 * Default is 400Mhz (system_pll1_800m with div = 2) set by ROM for ND VDD_SOC
	 */
	clock_enable(CCGR_GIC, 0);
	clock_set_target_val(GIC_CLK_ROOT, CLK_ROOT_ON | CLK_ROOT_SOURCE_SEL(5));
	clock_enable(CCGR_GIC, 1);

	puts("Normal Boot\n");

	/* Copy EEPROM contents to DRAM */
	memcpy(ep, &eeprom, sizeof(*ep));
	memcpy(carrier_ep, &carrier_eeprom, sizeof(*carrier_ep));
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	int board_id = var_detect_board_id();

	if ((board_id == BOARD_ID_DART) && !strcmp(name, "imx8mp-var-dart-dt8mcustomboard")) {
		return 0;
	} else if ((board_id == BOARD_ID_SOM) && !strcmp(name, "imx8mp-var-som-symphony")) {
		return 0;
	} else if ((board_id == BOARD_ID_SMARC) && !strcmp(name, "imx8mp-var-smarc-echo")) {
		return 0;
	}

	return -1;
}
#endif

#ifdef CONFIG_POWER
static struct i2c_pads_info i2c_pads_dart = {
	.scl = {
		.i2c_mode = MX8MP_PAD_I2C1_SCL__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_I2C1_SCL__GPIO5_IO14 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_I2C1_SDA__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_I2C1_SDA__GPIO5_IO15 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(5, 15),
	},
};

static struct i2c_pads_info i2c_pads_som = {
	.scl = {
		.i2c_mode = MX8MP_PAD_SD1_DATA4__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_SD1_DATA4__GPIO2_IO06 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(2, 6),
	},
	.sda = {
		.i2c_mode = MX8MP_PAD_SD1_DATA5__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gpio_mode = MX8MP_PAD_SD1_DATA5__GPIO2_IO07 | MUX_PAD_CTRL(I2C_PAD_CTRL),
		.gp = IMX_GPIO_NR(2, 7),
	},
};
#endif

void board_init_f(ulong dummy)
{
	int ret;
	struct udevice *dev;
	int board_id;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	timer_init();

	ret = spl_early_init();
	if (ret)
		hang();

	/* UART can be initialized only after DM setup in spl_early_init().
	 * SOM and DART have different debug UARTs, board detection code
	 * uses GPIO, which can be accessed only after DM is initialized.
	 */
	spl_uart_init();

	board_early_init_f();

	/* Can run only after UART clock is enabled */
	preloader_console_init();

	ret = uclass_get_device_by_name(UCLASS_CLK,
					"clock-controller@30380000",
					&dev);
	if (ret < 0) {
		printf("Failed to find clock node. Check device tree\n");
		hang();
	}

	enable_tzc380();

	board_id = var_detect_board_id();

	if (board_id == BOARD_ID_SMARC) {
		/* Force PWM lines low to keep panel backlight off */
		imx_iomux_v3_setup_multiple_pads(lvds_bklt_pwm_pads,
						 ARRAY_SIZE(lvds_bklt_pwm_pads));

		gpio_request(LVDS0_BKLT_PWM, "lvds0_bklt_pwm");
		gpio_request(LVDS1_BKLT_PWM, "lvds1_bklt_pwm");
		gpio_direction_output(LVDS0_BKLT_PWM, 0);
		gpio_direction_output(LVDS1_BKLT_PWM, 0);
		gpio_free(LVDS0_BKLT_PWM);
		gpio_free(LVDS1_BKLT_PWM);
	}

#ifdef CONFIG_POWER
	/* I2C Bus 0 initialization - PMIC/SOM EEPROM */
	if (board_id == BOARD_ID_DART)
		/* DART-MX8M-PLUS */
		setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pads_dart);
	else
		/* VAR-SOM-MX8M-PLUS and VAR-SMARC-MX8M-PLUS */
		setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pads_som);
#endif

	if (board_id == BOARD_ID_DART)
		/* I2C Bus 1 initialization - DT8MCB Carrier EEPROM reading */
		setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c1_pads_dart);
	else if (board_id == BOARD_ID_SOM)
		/* I2C Bus 3 initialization - Symphony Carrier EEPROM reading */
		setup_i2c(3, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c4_pads_som);
	else
		/* I2C Bus 1 initialization - Echo Carrier EEPROM reading */
		setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c1_pads_smarc);

	/* PMIC initialization */
	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}
