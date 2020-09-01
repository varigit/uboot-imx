/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 * Copyright 2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <miiphy.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mq_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <splash.h>
#include <usb.h>
#include <dwc3-uboot.h>

#include "../common/imx8_eeprom.h"

extern int var_setup_mac(struct var_eeprom *eeprom);

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE)

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MQ_PAD_GPIO1_IO02__WDOG1_WDOG_B | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const uart_pads[] = {
	IMX8MQ_PAD_UART1_RXD__UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MQ_PAD_UART1_TXD__UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

int board_early_init_f(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

	init_uart_clk(0); /* Init UART0 clock */

	return 0;
}

#ifdef CONFIG_FEC_MXC
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1],
		IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_MASK, 0);
	return set_clk_enet(ENET_125MHZ);
}
#endif

#ifdef CONFIG_USB_DWC3

#define USB_PHY_CTRL0			0xF0040
#define USB_PHY_CTRL0_REF_SSP_EN	BIT(2)

#define USB_PHY_CTRL1			0xF0044
#define USB_PHY_CTRL1_RESET		BIT(0)
#define USB_PHY_CTRL1_COMMONONN		BIT(1)
#define USB_PHY_CTRL1_ATERESET		BIT(3)
#define USB_PHY_CTRL1_VDATSRCENB0	BIT(19)
#define USB_PHY_CTRL1_VDATDETENB0	BIT(20)

#define USB_PHY_CTRL2			0xF0048
#define USB_PHY_CTRL2_TXENABLEN0	BIT(8)

static struct dwc3_device dwc3_device_data = {
	.maximum_speed = USB_SPEED_HIGH,
	.base = USB1_BASE_ADDR,
	.dr_mode = USB_DR_MODE_PERIPHERAL,
	.index = 0,
	.power_down_scale = 2,
};

int usb_gadget_handle_interrupts(void)
{
	dwc3_uboot_handle_interrupt(0);
	return 0;
}

static void dwc3_nxp_usb_phy_init(struct dwc3_device *dwc3)
{
	u32 RegData;

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_VDATSRCENB0 | USB_PHY_CTRL1_VDATDETENB0 |
			USB_PHY_CTRL1_COMMONONN);
	RegData |= USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET;
	writel(RegData, dwc3->base + USB_PHY_CTRL1);

	RegData = readl(dwc3->base + USB_PHY_CTRL0);
	RegData |= USB_PHY_CTRL0_REF_SSP_EN;
	writel(RegData, dwc3->base + USB_PHY_CTRL0);

	RegData = readl(dwc3->base + USB_PHY_CTRL2);
	RegData |= USB_PHY_CTRL2_TXENABLEN0;
	writel(RegData, dwc3->base + USB_PHY_CTRL2);

	RegData = readl(dwc3->base + USB_PHY_CTRL1);
	RegData &= ~(USB_PHY_CTRL1_RESET | USB_PHY_CTRL1_ATERESET);
	writel(RegData, dwc3->base + USB_PHY_CTRL1);
}

#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)
int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);

	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
		return dwc3_uboot_init(&dwc3_device_data);
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
	}

	imx8m_usb_power(index, false);

	return 0;
}
#endif

int board_init(void)
{
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)
	init_usb_clk();
#endif

	return 0;
}

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];

#ifdef CONFIG_FEC_MXC
	var_setup_mac(ep);
#endif
	var_eeprom_print_prod_info(ep);

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "IMX8MQ-VAR-DART");
	env_set("board_rev", "iMX8MQ");
#endif

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}
