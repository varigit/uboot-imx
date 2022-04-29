/*
 * Copyright 2018 NXP
 * Copyright 2018-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <usb.h>
#include <dm.h>

#include "../common/imx8_eeprom.h"
#include "imx8mm_var_dart.h"

DECLARE_GLOBAL_DATA_PTR;

extern int var_setup_mac(struct var_eeprom *eeprom);

#define GPIO_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1 | PAD_CTL_PUE | PAD_CTL_PE)

#ifdef CONFIG_SPL_BUILD
#define ID_GPIO 	IMX_GPIO_NR(2, 11)

static iomux_v3_cfg_t const id_pads[] = {
	IMX8MM_PAD_SD1_STROBE_GPIO2_IO11 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

int get_board_id(void)
{
	static int board_id = UNKNOWN_BOARD;

	if (board_id != UNKNOWN_BOARD)
		return board_id;

	imx_iomux_v3_setup_multiple_pads(id_pads, ARRAY_SIZE(id_pads));
	gpio_request(ID_GPIO, "board_id");
	gpio_direction_input(ID_GPIO);

	board_id = gpio_get_value(ID_GPIO) ? DART_MX8M_MINI : VAR_SOM_MX8M_MINI;

	return board_id;
}
#else
int get_board_id(void)
{
	static int board_id = UNKNOWN_BOARD;

	if (board_id != UNKNOWN_BOARD)
		return board_id;

	if (of_machine_is_compatible("variscite,imx8mm-var-som"))
		board_id = VAR_SOM_MX8M_MINI;
	else if (of_machine_is_compatible("variscite,imx8mm-var-dart"))
		board_id = DART_MX8M_MINI;
	else
		board_id = UNKNOWN_BOARD;

	return board_id;
}
#endif

int var_get_som_rev(struct var_eeprom *ep)
{
	switch (ep->somrev) {
	case 0:
		return SOM_REV_10;
	case 1:
		return SOM_REV_11;
	case 2:
		return SOM_REV_12;
	case 3:
		return SOM_REV_13;
	default:
		return UNKNOWN_REV;
	}
}

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart1_pads[] = {
	IMX8MM_PAD_UART1_RXD_UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART1_TXD_UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart4_pads[] = {
	IMX8MM_PAD_UART4_RXD_UART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART4_TXD_UART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

extern struct mxc_uart *mxc_base;

int board_early_init_f(void)
{
	int id;
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	id = get_board_id();

	if (id == DART_MX8M_MINI) {
		init_uart_clk(0);
		imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
	}else if (id == VAR_SOM_MX8M_MINI) {
		init_uart_clk(3);
		mxc_base = (struct mxc_uart *)UART4_BASE_ADDR;
		imx_iomux_v3_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
	}

	return 0;
}

#ifdef CONFIG_FEC_MXC
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}
#endif

#ifdef CONFIG_CI_UDC
int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, false);

	return 0;
}
#endif

int board_init(void)
{
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

	return 0;
}

#define DART_CARRIER_DETECT_GPIO IMX_GPIO_NR(3, 14)

static iomux_v3_cfg_t const dart_carrier_detect_pads[] = {
	IMX8MM_PAD_NAND_DQS_GPIO3_IO14 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static int var_detect_dart_carrier_rev(void)
{
	static int dart_carrier_rev = DART_CARRIER_REV_UNDEF;

	imx_iomux_v3_setup_multiple_pads(dart_carrier_detect_pads,
				ARRAY_SIZE(dart_carrier_detect_pads));

	gpio_request(DART_CARRIER_DETECT_GPIO, "dart_carrier_detect");
	gpio_direction_input(DART_CARRIER_DETECT_GPIO);

	if (gpio_get_value(DART_CARRIER_DETECT_GPIO))
		dart_carrier_rev = DART_CARRIER_REV_1;
	else
		dart_carrier_rev = DART_CARRIER_REV_2;

	return dart_carrier_rev;
}

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	int som_rev;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	int id = get_board_id();
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	struct var_carrier_eeprom carrier_eeprom;
	char carrier_rev[16] = {0};

#ifdef CONFIG_FEC_MXC
	var_setup_mac(ep);
#endif
	var_eeprom_print_prod_info(ep);

	som_rev = var_get_som_rev(ep);

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	if (id == VAR_SOM_MX8M_MINI) {
		env_set("board_name", "VAR-SOM-MX8M-MINI");
		env_set("console", "ttymxc3,115200");
		switch (som_rev) {
		case SOM_REV_10:
			env_set("som_rev", "som_rev10");
			break;
		case SOM_REV_11:
			env_set("som_rev", "som_rev11");
			break;
		case SOM_REV_12:
			env_set("som_rev", "som_rev12");
			break;
		case SOM_REV_13:
			env_set("som_rev", "som_rev13");
			break;
		}
		var_carrier_eeprom_read(CARRIER_EEPROM_BUS_SOM, CARRIER_EEPROM_ADDR, &carrier_eeprom);
		var_carrier_eeprom_get_revision(&carrier_eeprom, carrier_rev, sizeof(carrier_rev));
		env_set("carrier_rev", carrier_rev);
	}
	else if (id == DART_MX8M_MINI) {

		int carrier_rev = var_detect_dart_carrier_rev();

		env_set("board_name", "DART-MX8M-MINI");

		if (carrier_rev == DART_CARRIER_REV_2)
			env_set("carrier_rev", "dt8m-2.x");
		else
			env_set("carrier_rev", "legacy");
	}

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY

#define BACK_KEY IMX_GPIO_NR(4, 6)
#define BACK_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const back_pads[] = {
	IMX8MM_PAD_SAI1_RXD4_GPIO4_IO6 | MUX_PAD_CTRL(BACK_PAD_CTRL),
};

int is_recovery_key_pressing(void)
{
	imx_iomux_v3_setup_multiple_pads(back_pads, ARRAY_SIZE(back_pads));
	gpio_request(BACK_KEY, "BACK");
	gpio_direction_input(BACK_KEY);
	if (gpio_get_value(BACK_KEY) == 0) { /* BACK key is low assert */
		printf("Recovery key pressed\n");
		return 1;
	}
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
