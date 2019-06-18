/*
 * Copyright 2018 NXP
 * Copyright 2019 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <miiphy.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <usb.h>

#include "../common/imx8m_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
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

#define FEC_RST_GPIO IMX_GPIO_NR(1, 9)
#define FEC_PWR_GPIO IMX_GPIO_NR(1, 7)
#define FEC_GPIO_PAD_CTRL (PAD_CTL_PUE | PAD_CTL_DSE1)

static iomux_v3_cfg_t const fec_rst_pads[] = {
	IMX8MM_PAD_GPIO1_IO09_GPIO1_IO9 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO07_GPIO1_IO7 | MUX_PAD_CTRL(FEC_GPIO_PAD_CTRL),
};

static int setup_mac(struct var_eeprom *eeprom)
{
	int ret;
	unsigned char enetaddr[6];

	ret = eth_env_get_enetaddr("ethaddr", enetaddr);
	if (ret)
		return 0;

	ret = var_eeprom_get_mac(eeprom, enetaddr);
	if (ret)
		return ret;

	if (!is_valid_ethaddr(enetaddr))
		return -1;

	return eth_env_set_enetaddr("ethaddr", enetaddr);
}

static void setup_iomux_fec(void)
{
	imx_iomux_v3_setup_multiple_pads(fec_rst_pads,
					 ARRAY_SIZE(fec_rst_pads));

	/* Power-up ethernet PHY */
	gpio_request(FEC_PWR_GPIO, "phy_pwr");
	gpio_direction_output(FEC_PWR_GPIO, 0);
	mdelay(10);

	/* Reset ethernet PHY */
	gpio_request(FEC_RST_GPIO, "phy_rst");
	gpio_direction_output(FEC_RST_GPIO, 0);
	mdelay(10);
	gpio_direction_output(FEC_RST_GPIO, 1);
}

static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *const iomuxc_gpr_regs
		= (struct iomuxc_gpr_base_regs *) IOMUXC_GPR_BASE_ADDR;

	setup_iomux_fec();

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&iomuxc_gpr_regs->gpr[1],
			IOMUXC_GPR_GPR1_GPR_ENET1_TX_CLK_SEL_SHIFT, 0);
	return set_clk_enet(ENET_125MHZ);
}

#define AR803x_PHY_DEBUG_ADDR_REG	0x1d
#define AR803x_PHY_DEBUG_DATA_REG	0x1e

#define AR803x_DEBUG_REG_0		0x00
#define AR803x_DEBUG_REG_5		0x05

int board_phy_config(struct phy_device *phydev)
{
	/* Disable RGMII RX clock delay (enabled by default) */
	phy_write(phydev, MDIO_DEVAD_NONE, AR803x_PHY_DEBUG_ADDR_REG,
		  AR803x_DEBUG_REG_0);
	phy_write(phydev, MDIO_DEVAD_NONE, AR803x_PHY_DEBUG_DATA_REG, 0);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

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

int board_mmc_get_env_dev(int devno)
{
	return devno - 1;
}

int mmc_map_to_kernel_blk(int devno)
{
	return devno + 1;
}

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	struct var_eeprom eeprom;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];

	var_eeprom_read_header(&eeprom);

#ifdef CONFIG_FEC_MXC
	setup_mac(&eeprom);
#endif
	var_eeprom_print_prod_info(&eeprom);

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

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
