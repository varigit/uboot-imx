// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 * Copyright 2020 Variscite Ltd.
 */

#include <common.h>
#include <errno.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mp_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <spl.h>
#include <asm/mach-imx/dma.h>
#include <power/pmic.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <power/regulator.h>

#include "../common/imx8_eeprom.h"
#include "imx8mp_var_dart.h"

extern int var_setup_mac(struct var_eeprom *ep);

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_BUILD
#define GPIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_DSE1 | PAD_CTL_PUE | PAD_CTL_PE)
#define BOARD_DETECT_GPIO IMX_GPIO_NR(2, 11)

static iomux_v3_cfg_t const board_detect_pads[] = {
	MX8MP_PAD_SD1_STROBE__GPIO2_IO11 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};
#endif

int var_detect_board_id(void)
{
	static int board_id = BOARD_ID_UNDEF;

	if (board_id != BOARD_ID_UNDEF)
		return board_id;

#ifdef CONFIG_SPL_BUILD
	imx_iomux_v3_setup_multiple_pads(board_detect_pads,
				ARRAY_SIZE(board_detect_pads));

	gpio_request(BOARD_DETECT_GPIO, "board_detect");
	gpio_direction_input(BOARD_DETECT_GPIO);

	if (gpio_get_value(BOARD_DETECT_GPIO))
		board_id = BOARD_ID_SOM;
	else
		board_id = BOARD_ID_DART;
#else
	if (of_machine_is_compatible("variscite,imx8mp-var-som"))
		board_id = BOARD_ID_SOM;
	else if (of_machine_is_compatible("variscite,imx8mp-var-dart"))
		board_id = BOARD_ID_DART;
#endif

	return board_id;
}

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads_dart[] = {
	MX8MP_PAD_UART1_RXD__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX8MP_PAD_UART1_TXD__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart_pads_som[] = {
	MX8MP_PAD_UART2_RXD__UART2_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX8MP_PAD_UART2_TXD__UART2_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	MX8MP_PAD_GPIO1_IO02__WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

extern struct mxc_uart *mxc_base;

int board_early_init_f(void)
{
	int board_id;
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	board_id = var_detect_board_id();
	if (board_id == BOARD_ID_DART) {
		imx_iomux_v3_setup_multiple_pads(uart_pads_dart,
			ARRAY_SIZE(uart_pads_dart));
		init_uart_clk(0);
	}
	else {
		imx_iomux_v3_setup_multiple_pads(uart_pads_som,
			ARRAY_SIZE(uart_pads_som));
		init_uart_clk(1);
		mxc_base = (struct mxc_uart *)UART2_BASE_ADDR;
	}

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
#ifdef CONFIG_IMX8M_DRAM_INLINE_ECC
	int rc;
	phys_addr_t ecc0_start = 0xb0000000;
	phys_addr_t ecc1_start = 0x130000000;
	phys_addr_t ecc2_start = 0x1b0000000;
	size_t ecc_size = 0x10000000;

	rc = add_res_mem_dt_node(blob, "ecc", ecc0_start, ecc_size);
	if (rc < 0) {
		printf("Could not create ecc0 reserved-memory node.\n");
		return rc;
	}

	rc = add_res_mem_dt_node(blob, "ecc", ecc1_start, ecc_size);
	if (rc < 0) {
		printf("Could not create ecc1 reserved-memory node.\n");
		return rc;
	}

	rc = add_res_mem_dt_node(blob, "ecc", ecc2_start, ecc_size);
	if (rc < 0) {
		printf("Could not create ecc2 reserved-memory node.\n");
		return rc;
	}
#endif

	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Enable RGMII TX clk output */
	setbits_le32(&gpr->gpr[1], BIT(22));

	return 0;
}
#endif

#ifdef CONFIG_DWC_ETH_QOS

#define EQOS_PWR_GPIO_DART IMX_GPIO_NR(2, 20)
#define EQOS_RST_GPIO_DART IMX_GPIO_NR(2, 11)

#define EQOS_PWR_GPIO_SOM IMX_GPIO_NR(2, 20)
#define EQOS_RST_GPIO_SOM IMX_GPIO_NR(1, 10)

static iomux_v3_cfg_t const eqos_pads_dart[] = {
	MX8MP_PAD_SD2_WP__GPIO2_IO20 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX8MP_PAD_SD1_STROBE__GPIO2_IO11 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const eqos_pads_som[] = {
	MX8MP_PAD_SD2_WP__GPIO2_IO20 | MUX_PAD_CTRL(NO_PAD_CTRL),
	MX8MP_PAD_GPIO1_IO10__GPIO1_IO10 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_eqos(void)
{
	int pwr_gpio, rst_gpio;
	int board_id = var_detect_board_id();

	if (board_id == BOARD_ID_DART) {
		imx_iomux_v3_setup_multiple_pads(eqos_pads_dart,
					 ARRAY_SIZE(eqos_pads_dart));
		pwr_gpio = EQOS_PWR_GPIO_DART;
		rst_gpio = EQOS_RST_GPIO_DART;
	} else {
		imx_iomux_v3_setup_multiple_pads(eqos_pads_som,
					 ARRAY_SIZE(eqos_pads_som));
		pwr_gpio = EQOS_PWR_GPIO_SOM;
		rst_gpio = EQOS_RST_GPIO_SOM;
	}

	/* Power up EQOS PHY */
	gpio_request(pwr_gpio, "eqos_pwr");
	gpio_direction_output(pwr_gpio, 0);
	mdelay(20);
	gpio_direction_output(pwr_gpio, 1);
	mdelay(100);

	/* Reset EQOS PHY */
	gpio_request(rst_gpio, "eqos_rst");
	gpio_direction_output(rst_gpio, 0);
	mdelay(10);
	gpio_direction_output(rst_gpio, 1);
	mdelay(100);
}

static int setup_eqos(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	setup_iomux_eqos();

	/* set INTF as RGMII, enable RGMII TXC clock */
	clrsetbits_le32(&gpr->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET_QOS_INTF_SEL_MASK, BIT(16));
	setbits_le32(&gpr->gpr[1], BIT(19) | BIT(21));

	return set_clk_eqos(ENET_125MHZ);
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

#define USB_PHY_CTRL6			0xF0058

#define HSIO_GPR_BASE					(0x32F10000U)
#define HSIO_GPR_REG_0					(HSIO_GPR_BASE)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT	(1)
#define HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN		(0x1U << HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN_SHIFT)


static struct dwc3_device dwc3_device_data = {
#ifdef CONFIG_SPL_BUILD
	.maximum_speed = USB_SPEED_HIGH,
#else
	.maximum_speed = USB_SPEED_SUPER,
#endif
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

	/* enable usb clock via hsio gpr */
	RegData = readl(HSIO_GPR_REG_0);
	RegData |= HSIO_GPR_REG_0_USB_CLOCK_MODULE_EN;
	writel(RegData, HSIO_GPR_REG_0);

	/* USB3.0 PHY signal fsel for 100M ref */
	RegData = readl(dwc3->base + USB_PHY_CTRL0);
	RegData = (RegData & 0xfffff81f) | (0x2a<<5);
	writel(RegData, dwc3->base + USB_PHY_CTRL0);

	RegData = readl(dwc3->base + USB_PHY_CTRL6);
	RegData &=~0x1;
	writel(RegData, dwc3->base + USB_PHY_CTRL6);

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
	int ret = 0;
	imx8m_usb_power(index, true);

	if (index == 0 && init == USB_INIT_DEVICE) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_ufp_mode(&port1);
		if (ret)
			return ret;
#endif
		dwc3_nxp_usb_phy_init(&dwc3_device_data);
		return dwc3_uboot_init(&dwc3_device_data);
	} else if (index == 0 && init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_setup_dfp_mode(&port1);
#endif
		return ret;
	}

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;
	if (index == 0 && init == USB_INIT_DEVICE) {
		dwc3_uboot_exit(index);
	} else if (index == 0 && init == USB_INIT_HOST) {
#ifdef CONFIG_USB_TCPC
		ret = tcpc_disable_src_vbus(&port1);
#endif
	}

	imx8m_usb_power(index, false);

	return ret;
}

#endif

int board_init(void)
{
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_DWC_ETH_QOS
	setup_eqos();
#endif

#if defined(CONFIG_USB_DWC3) || defined(CONFIG_USB_XHCI_IMX8M)
	init_usb_clk();
#endif

	return 0;
}

#define SDRAM_SIZE_STR_LEN 5

int board_late_init(void)
{
	int board_id;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	struct var_eeprom *ep = VAR_EEPROM_DATA;

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	env_set("board_name", "DART-MX8MP");
	env_set("board_rev", "iMX8MP");
#endif

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d",
			(int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	board_id = var_detect_board_id();
	if (board_id == BOARD_ID_SOM) {
		env_set("board_name", "VAR-SOM-MX8M-PLUS");
		env_set("console", "ttymxc1,115200");
	}
	else if (board_id == BOARD_ID_DART) {
		env_set("board_name", "DART-MX8M-PLUS");
	}

	var_setup_mac(ep);
	var_eeprom_print_prod_info(ep);

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
	return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif

#ifdef CONFIG_SPL_MMC_SUPPORT

#define UBOOT_RAW_SECTOR_OFFSET 0x40
unsigned long spl_mmc_get_uboot_raw_sector(struct mmc *mmc)
{
	u32 boot_dev = spl_boot_device();
	switch (boot_dev) {
		case BOOT_DEVICE_MMC2:
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR - UBOOT_RAW_SECTOR_OFFSET;
		default:
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
	}
}
#endif
