/*
 * Copyright 2018 NXP
 * Copyright 2019 Variscite Ltd.
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

DECLARE_GLOBAL_DATA_PTR;

extern int var_setup_mac(struct var_eeprom *eeprom);

enum {
	DART_MX8M_MINI,
	VAR_SOM_MX8M_MINI,
	UNKNOWN_BOARD,
};

enum {
	SOM_REV10,
	SOM_REV11,
	UNKNOWN_REV,
};

static int get_board_id(void)
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

static int get_som_rev(void)
{
	struct var_eeprom eeprom = {0};
	static int som_rev = UNKNOWN_REV;

	if (som_rev != UNKNOWN_REV)
		return som_rev;

	var_eeprom_read_header(&eeprom);

	if (!var_eeprom_is_valid(&eeprom))
		som_rev = SOM_REV10;
	else if (eeprom.somrev == 0)
		som_rev = SOM_REV10;
	else
		som_rev = SOM_REV11;

	return som_rev;
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

#ifdef CONFIG_BOARD_POSTCLK_INIT
int board_postclk_init(void)
{
	/* TODO */
	return 0;
}
#endif

/* Return DRAM size in bytes */
static unsigned long long get_dram_size(void)
{
	int ret;
	u32 dram_size_mb;
	struct var_eeprom eeprom;

	var_eeprom_read_header(&eeprom);
	ret = var_eeprom_get_dram_size(&eeprom, &dram_size_mb);
	if (ret)
		return (1ULL * DEFAULT_DRAM_SIZE_MB ) << 20;

	return (1ULL * dram_size_mb) << 20;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_dram_size();

	return 0;
}

int dram_init(void)
{
	unsigned long long mem_size, max_low_size, dram_size;

	max_low_size = 0x100000000ULL - CONFIG_SYS_SDRAM_BASE;
	dram_size = get_dram_size();

	if (dram_size > max_low_size)
		mem_size = max_low_size;
	else
		mem_size = dram_size;

	/* rom_pointer[1] contains the size of TEE occupies */
	gd->ram_size = mem_size - rom_pointer[1];

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return 0;
}
#endif

#ifdef CONFIG_FEC_MXC
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_SHIFT, 0);
	return set_clk_enet(ENET_125MHZ);
}
#endif

#define USB_OTG1_ID_GPIO  IMX_GPIO_NR(1, 10)

static iomux_v3_cfg_t const usb_pads[] = {
	IMX8MM_PAD_GPIO1_IO10_GPIO1_IO10  | MUX_PAD_CTRL(NO_PAD_CTRL),
};

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

/* Used only on VAR-SOM-MX8M-MINI Rev1.0 (with extcon) */
int board_ehci_usb_phy_mode(struct udevice *dev)
{
	if (dev->seq == 1)
		return USB_INIT_HOST;
	else
		return gpio_get_value(USB_OTG1_ID_GPIO) ?
			USB_INIT_DEVICE : USB_INIT_HOST;
}

static void setup_usb(void)
{
	if ((get_board_id() == VAR_SOM_MX8M_MINI) &&
	    (get_som_rev() == SOM_REV10)) {
		imx_iomux_v3_setup_multiple_pads(usb_pads, ARRAY_SIZE(usb_pads));
		gpio_request(USB_OTG1_ID_GPIO, "usb_otg1_id");
		gpio_direction_input(USB_OTG1_ID_GPIO);
	}
}
#endif

int board_init(void)
{
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_CI_UDC
	setup_usb();
#endif

	return 0;
}

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	struct var_eeprom eeprom;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	int id = get_board_id();

	var_eeprom_read_header(&eeprom);

#ifdef CONFIG_FEC_MXC
	var_setup_mac(&eeprom);
#endif
	var_eeprom_print_prod_info(&eeprom);

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	if (id == VAR_SOM_MX8M_MINI) {
		env_set("board_name", "VAR-SOM-MX8M-MINI");
		env_set("console", "ttymxc3,115200 earlycon=ec_imx6q,0x30a60000,115200");
		if (get_som_rev() == SOM_REV10)
			env_set("som_rev", "som_rev10");
		else
			env_set("som_rev", "som_rev11");
	}
	else if (id == DART_MX8M_MINI)
		env_set("board_name", "DART-MX8M-MINI");

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY

#define BACK_KEY 	IMX_GPIO_NR(4, 6)
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
