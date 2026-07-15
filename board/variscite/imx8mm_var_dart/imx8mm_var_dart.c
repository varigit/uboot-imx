/*
 * Copyright 2018 NXP
 * Copyright 2018-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <env.h>
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
#include <dt-bindings/gpio/gpio.h>
#include <fdt_support.h>
#include <linux/libfdt.h>

#include "../common/extcon-ptn5150.h"
#include "../common/imx8_eeprom.h"
#include "imx8mm_var_dart.h"

DECLARE_GLOBAL_DATA_PTR;

#define SOM_REV_STR_LEN 16

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

#ifdef CONFIG_EXTCON_PTN5150
static struct extcon_ptn5150 usb_ptn5150;
#endif

int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);

#if (!defined(CONFIG_SPL_BUILD) && defined(CONFIG_EXTCON_PTN5150))
	if (index == 0) {
		/* Verify port is in proper mode */
		int phy_mode = extcon_ptn5150_phy_mode(&usb_ptn5150);

		//Only verify phy_mode if ptn5150 is initialized
		if (phy_mode >= 0 && phy_mode != init)
			return -ENODEV;
	}
#endif

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, false);

	return 0;
}

#ifdef CONFIG_EXTCON_PTN5150
int board_ehci_usb_phy_mode(struct udevice *dev)
{
	int usb_phy_mode = extcon_ptn5150_phy_mode(&usb_ptn5150);

	/* Default to host mode if not connected */
	if (usb_phy_mode < 0)
		usb_phy_mode = USB_INIT_HOST;

	return usb_phy_mode;
}
#endif
#endif

int board_init(void)
{
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

	return 0;
}

#if defined(CONFIG_OF_BOARD_SETUP) || defined(CONFIG_OF_BOARD_FIXUP)
static bool fdt_is_dart_mx8m_mini(void *blob)
{
	const char *model;

	model = fdt_getprop(blob, 0, "model", NULL);
	if (model && strstr(model, "DART-MX8M-MINI"))
		return true;

	return false;
}

static bool is_som_rev_3_0(struct var_eeprom *ep)
{
	return var_eeprom_is_valid(ep) &&
	       SOMREV_MAJOR(ep->somrev) == 3 &&
	       SOMREV_MINOR(ep->somrev) == 0;
}

static int fdt_setprop_empty_with_resize(void *blob, int node,
					 const char *name)
{
	int ret;

	ret = fdt_setprop_empty(blob, node, name);
	if (ret != -FDT_ERR_NOSPACE)
		return ret;

	ret = fdt_increase_size(blob, 128);
	if (ret)
		return ret;

	return fdt_setprop_empty(blob, node, name);
}

static int fixup_fdt_eth_phy_regulator(void *blob)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	const fdt32_t *gpio;
	fdt32_t new_gpio[3];
	int len, node, ret;

	if (!fdt_is_dart_mx8m_mini(blob))
		return 0;

	if (!is_som_rev_3_0(ep))
		return 0;

	node = fdt_path_offset(blob, "/regulator-eth-phy");
	if (node < 0) {
		printf("Could not find Ethernet PHY regulator node: %s\n",
		       fdt_strerror(node));
		return 0;
	}

	gpio = fdt_getprop(blob, node, "gpio", &len);
	if (!gpio || len < sizeof(new_gpio)) {
		printf("Invalid Ethernet PHY regulator GPIO property\n");
		return 0;
	}

	new_gpio[0] = gpio[0];
	new_gpio[1] = gpio[1];
	new_gpio[2] = cpu_to_fdt32(GPIO_ACTIVE_HIGH);

	ret = fdt_setprop_inplace(blob, node, "gpio", new_gpio,
				  sizeof(new_gpio));
	if (ret) {
		printf("Could not update Ethernet PHY regulator GPIO: %s\n",
		       fdt_strerror(ret));
		return ret;
	}

	if (!fdt_get_property(blob, node, "enable-active-high", NULL)) {
		ret = fdt_setprop_empty_with_resize(blob, node,
						    "enable-active-high");
		if (ret)
			printf("Could not add Ethernet PHY regulator enable-active-high: %s\n",
			       fdt_strerror(ret));
	}

	printf("Applied SOM rev 3.0 Ethernet PHY regulator GPIO polarity fixup\n");

	return 0;
}
#endif

#ifdef CONFIG_OF_BOARD_FIXUP
int imx8m_board_fix_fdt(void *blob)
{
	return fixup_fdt_eth_phy_regulator(blob);
}
#endif

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return fixup_fdt_eth_phy_regulator(blob);
}
#endif

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	int id = get_board_id();
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	struct var_carrier_eeprom carrier_eeprom;
	char carrier_rev[CARRIER_REV_LEN] = {0};
	char som_rev[SOM_REV_STR_LEN] = {0};

#ifdef CONFIG_EXTCON_PTN5150
	extcon_ptn5150_setup(&usb_ptn5150);
#endif

#ifdef CONFIG_FEC_MXC
	var_setup_mac(ep);
#endif
	var_eeprom_print_prod_info(ep);

	/* SoM Rev ENV*/
	snprintf(som_rev, sizeof(som_rev), "%ld.%ld", SOMREV_MAJOR(ep->somrev), SOMREV_MINOR(ep->somrev));
	env_set("som_rev", som_rev);

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	if (id != UNKNOWN_BOARD) {
		/* SoM Features ENV */
		env_set("som_has_wbe", (ep->features & VAR_EEPROM_F_WBE) ? "1" : "0");

		if (id == VAR_SOM_MX8M_MINI) {
			env_set("board_name", "VAR-SOM-MX8M-MINI");
			env_set("console", "ttymxc3,115200");
			var_carrier_eeprom_read(CARRIER_EEPROM_BUS_SOM, CARRIER_EEPROM_ADDR, &carrier_eeprom);
		}
		else if (id == DART_MX8M_MINI) {
			env_set("board_name", "DART-MX8M-MINI");
			var_carrier_eeprom_read(CARRIER_EEPROM_BUS_DART, CARRIER_EEPROM_ADDR, &carrier_eeprom);
		}

		var_carrier_eeprom_get_revision(&carrier_eeprom, carrier_rev, sizeof(carrier_rev));
		env_set("carrier_rev", carrier_rev);
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
	return 0;
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/
