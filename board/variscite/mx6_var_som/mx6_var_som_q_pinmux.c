/*
 * Copyright (C) 2012 Variscite, Ltd.
 * Author: Ron Donio <ron.d@variscite.com>
 *
 * Copyright (C) 2012-2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * 
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#ifdef CONFIG_MX6DL
#undef CONFIG_MX6DL
#endif

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include "mx6_ddr_regs.h"

#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>

#ifdef CONFIG_MX6DL
#undef CONFIG_MX6DL
#endif
#ifndef CONFIG_MX6Q
#define CONFIG_MX6Q
#endif
#include <asm/arch/mx6-pins.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PUS_47K_UP |			\
	PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define BUTTON_PAD_CTRL (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS)

#define I2C_PAD_CTRL	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define WEAK_PULLUP	(PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_SRE_SLOW)

#define WEAK_PULLDOWN	(PAD_CTL_PUS_100K_DOWN |		\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_HYS | PAD_CTL_SRE_SLOW)

#define OUTPUT_40OHM (PAD_CTL_SPEED_MED|PAD_CTL_DSE_40ohm)

/*
 * Pin mux defintion.
 * Define 2 sets one for MX6Q/MX6D and one for MX6DL/MX6solo
 *
 */
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)

/* */
struct i2c_pads_info i2c_pad_info0q = {
	.scl = {
		.i2c_mode = MX6_PAD_CSI0_DAT9__I2C1_SCL | PC,
		.gpio_mode = MX6_PAD_CSI0_DAT9__GPIO5_IO27 | PC,
		.gp = IMX_GPIO_NR(5, 27)
	},
	.sda = {
		.i2c_mode = MX6_PAD_CSI0_DAT8__I2C1_SDA | PC,
		.gpio_mode = MX6_PAD_CSI0_DAT8__GPIO5_IO26 | PC,
		.gp = IMX_GPIO_NR(5, 26)
	}
};

struct i2c_pads_info i2c_pad_info1q = {
	.scl = {
		.i2c_mode = MX6_PAD_KEY_COL3__I2C2_SCL | PC,
		.gpio_mode = MX6_PAD_KEY_COL3__GPIO4_IO12 | PC,
		.gp = IMX_GPIO_NR(4, 12)
	},
	.sda = {
		.i2c_mode = MX6_PAD_KEY_ROW3__I2C2_SDA | PC,
		.gpio_mode = MX6_PAD_KEY_ROW3__GPIO4_IO13 | PC,
		.gp = IMX_GPIO_NR(4, 13)
	}
};

struct i2c_pads_info i2c_pad_info2q = {
	.scl = {
		.i2c_mode = MX6_PAD_GPIO_5__I2C3_SCL | PC,
		.gpio_mode = MX6_PAD_GPIO_5__GPIO1_IO05 | PC,
		.gp = IMX_GPIO_NR(1, 5)
	},
	.sda = {
		.i2c_mode = MX6_PAD_GPIO_16__I2C3_SDA | PC,
		.gpio_mode = MX6_PAD_GPIO_16__GPIO7_IO11 | PC,
		.gp = IMX_GPIO_NR(7, 11)
	}
};

iomux_v3_cfg_t const uart1_padsq[] = {
	MX6_PAD_CSI0_DAT10__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_CSI0_DAT11__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL),
};

/* Ethernet */
iomux_v3_cfg_t const enet_reset_padsq[] = {
	MX6_PAD_ENET_CRS_DV__GPIO1_IO25		| MUX_PAD_CTRL(0x88),	/* Reset */
};

iomux_v3_cfg_t const enet_padsq[] = {
	MX6_PAD_ENET_MDIO__ENET_MDIO		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_MDC__ENET_MDC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TXC__RGMII_TXC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD0__RGMII_TD0		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD1__RGMII_TD1		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD2__RGMII_TD2		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TD3__RGMII_TD3		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_TX_CTL__RGMII_TX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_ENET_REF_CLK__ENET_TX_CLK	| MUX_PAD_CTRL(NO_PAD_CTRL),

	/* pin 35 - 1 (PHY_AD2) on reset */
	MX6_PAD_RGMII_RXC__GPIO6_IO30		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 32 - 1 - (MODE0) all */
	MX6_PAD_RGMII_RD0__GPIO6_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 31 - 1 - (MODE1) all */
	MX6_PAD_RGMII_RD1__GPIO6_IO27		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 28 - 1 - (MODE2) all */
	MX6_PAD_RGMII_RD2__GPIO6_IO28		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 27 - 1 - (MODE3) all */
	MX6_PAD_RGMII_RD3__GPIO6_IO29		| MUX_PAD_CTRL(NO_PAD_CTRL),
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
	MX6_PAD_RGMII_RX_CTL__GPIO6_IO24	| MUX_PAD_CTRL(NO_PAD_CTRL),
	MX6_PAD_ENET_CRS_DV__GPIO1_IO25		| MUX_PAD_CTRL(NO_PAD_CTRL),
};


iomux_v3_cfg_t const enet_pads_finalq[] = {
	MX6_PAD_RGMII_RXC__RGMII_RXC		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD0__RGMII_RD0		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD1__RGMII_RD1		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD2__RGMII_RD2		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RD3__RGMII_RD3		| MUX_PAD_CTRL(ENET_PAD_CTRL),
	MX6_PAD_RGMII_RX_CTL__RGMII_RX_CTL	| MUX_PAD_CTRL(ENET_PAD_CTRL),
};

iomux_v3_cfg_t const usdhc1_padsq[] = {							/* eMMC */
	MX6_PAD_SD1_CLK__SD1_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_CMD__SD1_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT0__SD1_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT1__SD1_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT2__SD1_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD1_DAT3__SD1_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};
iomux_v3_cfg_t const usdhc2_padsq[] = {							/* SDCARD */
	MX6_PAD_SD2_CLK__SD2_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_CMD__SD2_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT0__SD2_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT1__SD2_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT2__SD2_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD2_DAT3__SD2_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
};
iomux_v3_cfg_t const usdhc3_padsq[] = {							/* SDCARD */
	MX6_PAD_SD3_CLK__SD3_CLK	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__SD3_CMD	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT0__SD3_DATA0	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT1__SD3_DATA1	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT2__SD3_DATA2	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT3__SD3_DATA3	| MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

iomux_v3_cfg_t const pcie_padsq[] = {
	MX6_PAD_EIM_D21__GPIO3_IO21		| MUX_PAD_CTRL(NO_PAD_CTRL),		/* PCIE_DIS */
	MX6_PAD_EIM_D22__GPIO3_IO22		| MUX_PAD_CTRL(NO_PAD_CTRL),		/* PCIE_WAKEUP */
	MX6_PAD_DISP0_DAT19__GPIO5_IO13		| MUX_PAD_CTRL(NO_PAD_CTRL),	 	/* PCIE_RST */
	MX6_PAD_SD4_DAT4__GPIO2_IO12		| MUX_PAD_CTRL(NO_PAD_CTRL),		/* PCIE_PWR_ENA */
};

iomux_v3_cfg_t const di0_padsq[] = {
	MX6_PAD_DI0_DISP_CLK__IPU1_DI0_DISP_CLK,	/* DISP0_CLK */
	MX6_PAD_DI0_PIN2__IPU1_DI0_PIN02,		/* DISP0_HSYNC */
	MX6_PAD_DI0_PIN3__IPU1_DI0_PIN03,		/* DISP0_VSYNC */
};

iomux_v3_cfg_t const codec_padsq[] = {
	MX6_PAD_GPIO_19__GPIO4_IO05,
};

void setup_codec_padsq(void){
	imx_iomux_v3_setup_multiple_pads(codec_padsq, ARRAY_SIZE(codec_padsq));
}

void setup_pcie_padsq(void){
	imx_iomux_v3_setup_multiple_pads(pcie_padsq, ARRAY_SIZE(pcie_padsq));
}

void setup_enet_pads_resetq(void){
	imx_iomux_v3_setup_multiple_pads(enet_reset_padsq, ARRAY_SIZE(enet_reset_padsq));
}

void setup_enet_padsq(void){
	imx_iomux_v3_setup_multiple_pads(enet_padsq, ARRAY_SIZE(enet_padsq));
}
void setup_enet_pads_finalq(void){
	imx_iomux_v3_setup_multiple_pads(enet_pads_finalq, ARRAY_SIZE(enet_pads_finalq));
}
void setup_uart1_padsq(void){
	imx_iomux_v3_setup_multiple_pads(uart1_padsq, ARRAY_SIZE(uart1_padsq));
}
void setup_usdhc3_padsq (void){
	imx_iomux_v3_setup_multiple_pads(usdhc3_padsq, ARRAY_SIZE(usdhc3_padsq));
}
void setup_usdhc2_padsq (void){
	imx_iomux_v3_setup_multiple_pads(usdhc2_padsq, ARRAY_SIZE(usdhc2_padsq));
}
void setup_usdhc1_padsq (void){
	imx_iomux_v3_setup_multiple_pads(usdhc1_padsq, ARRAY_SIZE(usdhc1_padsq));
}
void setup_di0_padsq(void) {
	imx_iomux_v3_setup_multiple_pads(di0_padsq, ARRAY_SIZE(di0_padsq));
}

void setup_i2c_padq(void){
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info0q);
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1q);
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info2q);
}



