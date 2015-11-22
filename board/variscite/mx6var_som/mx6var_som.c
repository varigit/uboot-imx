/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 * Author: Jason Liu <r64343@freescale.com>
 *
 * Copyright (C) 2014 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite VAR_SOM_MX6 board.
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
 */

#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#if CONFIG_I2C_MXC
#include <i2c.h>
#include <asm/imx-common/mxc_i2c.h>
#endif
#include <mmc.h>
#include <fsl_esdhc.h>
#include <miiphy.h>
#include <micrel.h>
#include <netdev.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/crm_regs.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include "asm/arch/mx6_ddr_regs.h"

#include "mx6var_eeprom.h"

#if defined(CONFIG_MX6Q) || defined(CONFIG_MX6DL)
#error "This code was eliminated. ONLY SPL boot mode supported"
#endif
DECLARE_GLOBAL_DATA_PTR;
#define LOW_POWER_MODE_ENABLE 1

#define MX6QDL_SET_PAD(p, q) \
        if (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D)) \
                imx_iomux_v3_setup_pad(MX6Q_##p | q);\
        else \
                imx_iomux_v3_setup_pad(MX6DL_##p | q)

#define I2C_EXP_RST IMX_GPIO_NR(1, 15)
#define I2C3_STEER  IMX_GPIO_NR(5, 4)

#define VAR_SOM_BACKLIGHT_EN    IMX_GPIO_NR(4, 30)

#define VAR_SOM_MIPI_EN         IMX_GPIO_NR(3, 15)

#define UART_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |               \
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |            \
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |               \
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

#define ENET_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED   |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define PER_VCC_EN_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED   |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

#define SPI_PAD_CTRL (PAD_CTL_HYS |				\
	PAD_CTL_PUS_100K_DOWN | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm     | PAD_CTL_SRE_FAST)

#define WEIM_NOR_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST)


#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)

#define I2C_PAD_CTRL	(PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS |			\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

#define USB_PAD_CTRL	(PAD_CTL_PKE | PAD_CTL_PUE |		\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |		\
	PAD_CTL_DSE_40ohm | PAD_CTL_HYS | PAD_CTL_SRE_SLOW)


#if CONFIG_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)

#define I2C_PADS(name, scl_i2c, scl_gpio, scl_gp, sda_i2c, sda_gpio, sda_gp) \
	struct i2c_pads_info mx6q_##name = {		\
		.scl = {				\
			.i2c_mode = MX6Q_##scl_i2c,	\
			.gpio_mode = MX6Q_##scl_gpio,	\
			.gp = scl_gp,			\
		},					\
		.sda = {				\
			.i2c_mode = MX6Q_##sda_i2c,	\
			.gpio_mode = MX6Q_##sda_gpio,	\
			.gp = sda_gp,			\
		}					\
	};						\
	struct i2c_pads_info mx6s_##name = {		\
		.scl = {				\
			.i2c_mode = MX6DL_##scl_i2c,	\
			.gpio_mode = MX6DL_##scl_gpio,	\
			.gp = scl_gp,			\
		},					\
		.sda = {				\
			.i2c_mode = MX6DL_##sda_i2c,	\
			.gpio_mode = MX6DL_##sda_gpio,	\
			.gp = sda_gp,			\
		}					\
	};
I2C_PADS(i2c_pad_info1,
	 PAD_CSI0_DAT9__I2C1_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 PAD_CSI0_DAT9__GPIO_5_27 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 IMX_GPIO_NR(5, 27),
	 PAD_CSI0_DAT8__I2C1_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 PAD_CSI0_DAT8__GPIO_5_26 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 IMX_GPIO_NR(5, 26));

I2C_PADS(i2c_pad_info2,
	 PAD_KEY_COL3__I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 PAD_KEY_COL3__GPIO_4_12 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 IMX_GPIO_NR(4, 12),
	 PAD_KEY_ROW3__I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 PAD_KEY_ROW3__GPIO_4_13 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 IMX_GPIO_NR(4, 13));

I2C_PADS(i2c_pad_info3,
	 PAD_GPIO_5__I2C3_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 PAD_GPIO_5__GPIO_1_5 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 IMX_GPIO_NR(1, 5),
	 PAD_GPIO_16__I2C3_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 PAD_GPIO_16__GPIO_7_11 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	 IMX_GPIO_NR(7, 11));

#define I2C_PADS_INFO(name)	\
		(is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D)) ? \
						&mx6q_##name : &mx6s_##name

#endif

#ifdef CONFIG_USB_EHCI_MX6
static void setup_usb(void);
#endif

void p_udelay(int time)
{
	int i, j;

	for (i = 0; i < time; i++) {
		for (j = 0; j < 200; j++) {
			asm("nop");
			asm("nop");
		}
	}
}


static inline uint get_mmc_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
/*	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;
 	uint bt_mem_mmc = (soc_sbmr & 0x00001000) >> 12; */

	return bt_mem_ctl;
}


int dram_init(void){
volatile struct mmdc_p_regs *mmdc_p0;
ulong sdram_size, sdram_cs;
unsigned int volatile * const port1 = (unsigned int *) PHYS_SDRAM;
unsigned int volatile * port2;
unsigned int *sdram_global;

	mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	sdram_cs = mmdc_p0->mdasp;
	sdram_size = 1024;

	switch(sdram_cs) {
	        case 0x0000000f:
	            sdram_size = 256;
	            break;
		case 0x00000017:
			sdram_size = 512;
			break;
		case 0x00000027:
			sdram_size = 1024;
			break;
		case 0x00000047:
			sdram_size = 2048;
			break;
		case 0x00000087:
			sdram_size = 3840;
			break;
	        default:
			sdram_size = 1024;
	}

	if (is_cpu_pop_package()){
		sdram_size = 1024;
		gd->ram_size = ((ulong)sdram_size * 1024 * 1024);
		return 0;
	}

	sdram_global =  (u32 *)0x917000;
	if (*sdram_global  > sdram_size) sdram_size = *sdram_global;

	do {
		if (sdram_size == 3840) break;
		port2 = (unsigned int volatile *) (PHYS_SDRAM + ((sdram_size * 1024 * 1024) / 2));

		*port2 = 0;				// write zero to start of second half of memory.
		*port1 = 0x3f3f3f3f;	// write pattern to start of memory.

		if ((0x3f3f3f3f == *port2) && (sdram_size > 256))
			sdram_size = sdram_size / 2;	// Next step devide size by half
		else

		if (0 == *port2)		// Done actual size found.
			break;

	} while (sdram_size > 256);

	gd->ram_size = ((ulong)sdram_size * 1024 * 1024);

	return 0;
}

void codec_reset(int rst)
{
    MX6QDL_SET_PAD(PAD_GPIO_19__GPIO_4_5, MUX_PAD_CTRL(NO_PAD_CTRL));
	gpio_direction_output(IMX_GPIO_NR(4, 5), 1);   /* Variscite codec reset */

	if (rst) {
		printf("CODEC RESET\n");
		gpio_set_value(IMX_GPIO_NR(4, 5), 0);
	}
	else {
		printf("CODEC NORMAL\n");
		gpio_set_value(IMX_GPIO_NR(4, 5), 1);
	}
}


static void setup_iomux_enet(void)
{
	gpio_direction_output(IMX_GPIO_NR(1, 25), 0);	/* Variscite SOM PHY reset */
	gpio_direction_output(IMX_GPIO_NR(6, 30), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 25), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 27), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 28), 1);
	gpio_direction_output(IMX_GPIO_NR(6, 29), 1);


	MX6QDL_SET_PAD(PAD_ENET_MDIO__ENET_MDIO				, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_ENET_MDC__ENET_MDC				, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_TXC__ENET_RGMII_TXC			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_TD0__ENET_RGMII_TD0			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_TD1__ENET_RGMII_TD1			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_TD2__ENET_RGMII_TD2			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_TD3__ENET_RGMII_TD3			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_TX_CTL__RGMII_TX_CTL			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_ENET_REF_CLK__ENET_TX_CLK			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	/* pin 35 - 1 (PHY_AD2) on reset */
	MX6QDL_SET_PAD(PAD_RGMII_RXC__GPIO_6_30				, MUX_PAD_CTRL(NO_PAD_CTRL));
	/* pin 32 - 1 - (MODE0) all */
	MX6QDL_SET_PAD(PAD_RGMII_RD0__GPIO_6_25				, MUX_PAD_CTRL(NO_PAD_CTRL));
	/* pin 31 - 1 - (MODE1) all */
	MX6QDL_SET_PAD(PAD_RGMII_RD1__GPIO_6_27				, MUX_PAD_CTRL(NO_PAD_CTRL));
	/* pin 28 - 1 - (MODE2) all */
	MX6QDL_SET_PAD(PAD_RGMII_RD2__GPIO_6_28				, MUX_PAD_CTRL(NO_PAD_CTRL));
	/* pin 27 - 1 - (MODE3) all */
	MX6QDL_SET_PAD(PAD_RGMII_RD3__GPIO_6_29				, MUX_PAD_CTRL(NO_PAD_CTRL));
	/* pin 33 - 1 - (CLK125_EN) 125Mhz clockout enabled */
	MX6QDL_SET_PAD(PAD_RGMII_RX_CTL__GPIO_6_24				, MUX_PAD_CTRL(NO_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_ENET_CRS_DV__GPIO_1_25				, MUX_PAD_CTRL(NO_PAD_CTRL));

	gpio_direction_output(IMX_GPIO_NR(6, 24), 1);

	/* Need delay 10ms according to KSZ9021 spec */
	udelay(1000 * 10);

	// De-assert reset
	gpio_set_value(IMX_GPIO_NR(1, 25), 1);

	MX6QDL_SET_PAD(PAD_RGMII_RXC__ENET_RGMII_RXC			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_RD0__ENET_RGMII_RD0			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_RD1__ENET_RGMII_RD1			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_RD2__ENET_RGMII_RD2			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_RD3__ENET_RGMII_RD3			, MUX_PAD_CTRL(ENET_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_RGMII_RX_CTL__RGMII_RX_CTL			, MUX_PAD_CTRL(ENET_PAD_CTRL));
}


#ifdef CONFIG_I2C_MXC

static int setup_pmic_voltages(void)
{
	unsigned char value, rev_id = 0 ;


	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (i2c_probe(0x8))
		printf("Failed to probe PMIC error!!!\n");
	else {
		if (i2c_read(0x8, 0, 1, &value, 1)) {
			printf("Read device ID error!\n");
			return -1;
		}
		if (i2c_read(0x8, 3, 1, &rev_id, 1)) {
			printf("Read Rev ID error!\n");
			return -1;
		}
		printf("Found PFUZE100! deviceid=%x,revid=%x\n", value, rev_id);
		/*For camera streaks issue,swap VGEN5 and VGEN3 to power camera.
		*sperate VDDHIGH_IN and camera 2.8V power supply, after switch:
		*VGEN5 for VDDHIGH_IN and increase to 3V to align with datasheet
		*VGEN3 for camera 2.8V power supply
		*/
		int retval=0;

		if (!is_cpu_pop_package()){
			/* Set Gigbit Ethernet voltage (SOM v1.1/1.0)*/
			value = 0x60;
			if (i2c_write(0x8, 0x4a, 1, &value, 1)) {
				printf("Set Gigabit Ethernet voltage error!\n");
				return -1;
			}

			/*set VGEN3 to 2.5V*/
			value = 0x77;
			if (i2c_write(0x8, 0x6e, 1, &value, 1)) {
				printf("Set VGEN3 error!\n");
				return -1;
			}
			/*increase VGEN5 from 2.8 to 3V*/
			if (i2c_read(0x8, 0x70, 1, &value, 1)) {
				printf("Read VGEN5 error!\n");
				return -1;
			}
			value &= ~0xf;
			value |= 0xc;
			if (i2c_write(0x8, 0x70, 1, &value, 1)) {
				printf("Set VGEN5 error!\n");
				return -1;
			}
			/* set SW1AB staby volatage 0.975V*/
			if (i2c_read(0x8, 0x21, 1, &value, 1)) {
				printf("Read SW1ABSTBY error!\n");
				return -1;
			}
			value &= ~0x3f;
			value |= 0x1b;
			if (i2c_write(0x8, 0x21, 1, &value, 1)) {
				printf("Set SW1ABSTBY error!\n");
				return -1;
			}
			/* set SW1AB/VDDARM step ramp up time from 16us to 4us/25mV */
			if (i2c_read(0x8, 0x24, 1, &value, 1)) {
				printf("Read SW1ABSTBY error!\n");
				return -1;
			}
			value &= ~0xc0;
			value |= 0x40;
			if (i2c_write(0x8, 0x24, 1, &value, 1)) {
				printf("Set SW1ABSTBY error!\n");
				return -1;
			}

			/* set SW1C staby volatage 0.975V*/
			if (i2c_read(0x8, 0x2f, 1, &value, 1)) {
				printf("Read SW1CSTBY error!\n");
				return -1;
			}
			value &= ~0x3f;
			value |= 0x1b;
			if (i2c_write(0x8, 0x2f, 1, &value, 1)) {
				printf("Set SW1CSTBY error!\n");
				return -1;
			}

			/* set SW1C/VDDSOC step ramp up time to from 16us to 4us/25mV */
			if (i2c_read(0x8, 0x32, 1, &value, 1)) {
				printf("Read SW1ABSTBY error!\n");
				return -1;
			}
			value &= ~0xc0;
			value |= 0x40;
			if (i2c_write(0x8, 0x32, 1, &value, 1)) {
				printf("Set SW1ABSTBY error!\n");
				return -1;
			}
#if LOW_POWER_MODE_ENABLE
			//set low power mode voltages to disable
			//SW3ASTBY = 1.2750V
			value = 0x23;
			if (i2c_write(0x8, 0x3d, 1, &value, 1)) {
				printf("Set SW3ASTBY error!\n");
				return -1;
			}
			//SW3AOFF=1.2750V
			value = 0x23;
			if (i2c_write(0x8, 0x3e, 1, &value, 1)) {
				printf("Set SW3AOFF error!\n");
				return -1;
			}
			//SW2VOLT=3.2000V
			value = 0x70;
			if (i2c_write(0x8, 0x35, 1, &value, 1)) {
				printf("Set SW2VOLT error!\n");
				return -1;
			}
			//SW4MODE=OFF in standby
			value = 0x1;
			if (i2c_write(0x8, 0x4d, 1, &value, 1)) {
				printf("Set SW4MODE error!\n");
				return -1;
			}
			
			//VGEN3CTL = OFF in standby
			value = 0x37;
			if (i2c_write(0x8, 0x6e, 1, &value, 1)) {
				printf("Set VGEN3CTL error!\n");
				return -1;
			}
#endif			
		} else {

			printf("Set POP Voltage\n");

			value=0x29;
			retval+=i2c_write(0x8, 0x20, 1, &value, 1);
			value=0x18;
			retval+=i2c_write(0x8, 0x21, 1, &value, 1);
			retval+=i2c_write(0x8, 0x22, 1, &value, 1);

			value=0x29;
			retval+=i2c_write(0x8, 0x2E, 1, &value, 1);
			value=0x18;
			retval+=i2c_write(0x8, 0x2F, 1, &value, 1);
			retval+=i2c_write(0x8, 0x30, 1, &value, 1);
			
			value=0x72;
			retval+=i2c_write(0x8, 0x35, 1, &value, 1);
			value=0x70;
			retval+=i2c_write(0x8, 0x36, 1, &value, 1);
			retval+=i2c_write(0x8, 0x37, 1, &value, 1);
			
			value=0x0D;
			retval+=i2c_write(0x8, 0x23, 1, &value, 1);
			value=0x00;
			retval+=i2c_write(0x8, 0x24, 1, &value, 1);

			value=0x0D;
			retval+=i2c_write(0x8, 0x31, 1, &value, 1);
			value=0x40;
			retval+=i2c_write(0x8, 0x32, 1, &value, 1);
			
			value=0x0D;
			retval+=i2c_write(0x8, 0x38, 1, &value, 1);
			
			value=0x0F;
			retval+=i2c_write(0x8, 0x71, 1, &value, 1);
			if(retval)
			{
				printf("PMIC write voltages error!\n");
				return -1;
			}
		}
	}


	return 0;
}


#ifdef CONFIG_LDO_BYPASS_CHECK
void ldo_mode_set(int ldo_bypass)
{
	unsigned char value;
	ldo_bypass = 1;	/* ldo disabled on any Variscite SOM  */

	/* switch to ldo_bypass mode , boot on 800Mhz */
	if (ldo_bypass) {
		/* increase VDDARM/VDDSOC 1.42 */
		if (i2c_read(0x8, 0x20, 1, &value, 1)) {
			printf("Read SW1AB error!\n");
			return;
		}
		value &= ~0x3f;
		value |= 0x29;
		if (i2c_write(0x8, 0x20, 1, &value, 1)) {
			printf("Set SW1AB error!\n");
			return;
		}
		/* increase VDDARM/VDDSOC 1.42 */
		if (i2c_read(0x8, 0x2E, 1, &value, 1)) {
			printf("Read SW1C error!\n");
			return;
		}
		value &= ~0x3f;
		value |= 0x29;
		if (i2c_write(0x8, 0x2E, 1, &value, 1)) {
			printf("Set SW1C error!\n");
			return;
		}

		set_anatop_bypass();
		printf("switched to ldo_bypass mode!\n");
	}
}
#endif
#endif

static void setup_iomux_uart(void)
{
	MX6QDL_SET_PAD(PAD_CSI0_DAT10__UART1_TXD , MUX_PAD_CTRL(UART_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_CSI0_DAT11__UART1_RXD , MUX_PAD_CTRL(UART_PAD_CTRL));
}
void var_setup_iomux_per_vcc_en(void)
{
	MX6QDL_SET_PAD(PAD_EIM_D31__GPIO_3_31, MUX_PAD_CTRL(PER_VCC_EN_PAD_CTRL));
	gpio_direction_output(IMX_GPIO_NR(3, 31), 1);
	gpio_set_value(IMX_GPIO_NR(3, 31), 1);
}
#if 0
static void var_setup_iomux_som_solo_usb_en(void)
{
	MX6QDL_SET_PAD(PAD_KEY_ROW4__GPIO_4_15, MUX_PAD_CTRL(PER_VCC_EN_PAD_CTRL));
	gpio_direction_output(IMX_GPIO_NR(4, 15), 1);
	gpio_set_value(IMX_GPIO_NR(4, 15), 1);
//
	MX6QDL_SET_PAD(PAD_EIM_D22__GPIO_3_22, MUX_PAD_CTRL(PER_VCC_EN_PAD_CTRL));
	gpio_direction_output(IMX_GPIO_NR(3, 22), 1);
	gpio_set_value(IMX_GPIO_NR(3, 22), 1);

}
#endif
#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR},
	{USDHC1_BASE_ADDR},
};

struct fsl_esdhc_cfg usdhc_cfg_dart_emmc[2] = {
	{USDHC3_BASE_ADDR},
	{USDHC1_BASE_ADDR},
};

int board_mmc_getcd(struct mmc *mmc)
{
	return 1;
}

static int board_mmc_init_dart(bd_t *bis)
{
	s32 status = 0;


	MX6QDL_SET_PAD(PAD_SD3_CLK__USDHC3_CLK	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD3_CMD__USDHC3_CMD	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD3_DAT0__USDHC3_DAT0	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD3_DAT1__USDHC3_DAT1	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD3_DAT2__USDHC3_DAT2	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD3_DAT3__USDHC3_DAT3	, MUX_PAD_CTRL(USDHC_PAD_CTRL));

	usdhc_cfg_dart_emmc[0].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	usdhc_cfg_dart_emmc[0].max_bus_width = 4;
	status |= fsl_esdhc_initialize(bis, &usdhc_cfg_dart_emmc[0]);

	/* sdcard */
	MX6QDL_SET_PAD(PAD_SD2_CLK__USDHC2_CLK	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_CMD__USDHC2_CMD	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT0__USDHC2_DAT0	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT1__USDHC2_DAT1	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT2__USDHC2_DAT2	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT3__USDHC2_DAT3	, MUX_PAD_CTRL(USDHC_PAD_CTRL));

	usdhc_cfg_dart_emmc[1].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
	usdhc_cfg_dart_emmc[1].max_bus_width = 4;
	status |= fsl_esdhc_initialize(bis, &usdhc_cfg_dart_emmc[1]);


	return status;
}

int board_mmc_init(bd_t *bis)
{
	s32 status = 0;


	if (6 == get_mmc_boot_device())
		return(board_mmc_init_dart(bis));

	/* sdcard */
	MX6QDL_SET_PAD(PAD_SD2_CLK__USDHC2_CLK	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_CMD__USDHC2_CMD	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT0__USDHC2_DAT0	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT1__USDHC2_DAT1	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT2__USDHC2_DAT2	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD2_DAT3__USDHC2_DAT3	, MUX_PAD_CTRL(USDHC_PAD_CTRL));

	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
	usdhc_cfg[0].max_bus_width = 4;
	status |= fsl_esdhc_initialize(bis, &usdhc_cfg[0]);

	MX6QDL_SET_PAD(PAD_SD1_CLK__USDHC1_CLK	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD1_CMD__USDHC1_CMD	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD1_DAT0__USDHC1_DAT0	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD1_DAT1__USDHC1_DAT1	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD1_DAT2__USDHC1_DAT2	, MUX_PAD_CTRL(USDHC_PAD_CTRL));
	MX6QDL_SET_PAD(PAD_SD1_DAT3__USDHC1_DAT3	, MUX_PAD_CTRL(USDHC_PAD_CTRL));

	usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);
	usdhc_cfg[1].max_bus_width = 4;
	status |= fsl_esdhc_initialize(bis, &usdhc_cfg[1]);

	return status;
}

#endif




#ifdef CONFIG_SYS_USE_NAND

static void setup_gpmi_nand(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	/* config gpmi nand iomux */
	MX6QDL_SET_PAD(PAD_NANDF_CLE__RAWNAND_CLE		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_ALE__RAWNAND_ALE		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_WP_B__RAWNAND_RESETN	, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_RB0__RAWNAND_READY0	, MUX_PAD_CTRL(GPMI_PAD_CTRL0));
	MX6QDL_SET_PAD(PAD_NANDF_CS0__RAWNAND_CE0N		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_SD4_CMD__RAWNAND_RDN		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_SD4_CLK__RAWNAND_WRN		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D0__RAWNAND_D0		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D1__RAWNAND_D1		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D2__RAWNAND_D2		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D3__RAWNAND_D3		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D4__RAWNAND_D4		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D5__RAWNAND_D5		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D6__RAWNAND_D6		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_NANDF_D7__RAWNAND_D7		, MUX_PAD_CTRL(GPMI_PAD_CTRL2));
	MX6QDL_SET_PAD(PAD_SD4_DAT0__RAWNAND_DQS		, MUX_PAD_CTRL(GPMI_PAD_CTRL1));

	/* gate ENFC_CLK_ROOT clock first,before clk source switch */
	clrbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* config gpmi and bch clock to 100 MHz */
	clrsetbits_le32(&mxc_ccm->cs2cdr,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF(0) |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED(3) |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL(3));

	/* enable ENFC_CLK_ROOT clock */
	setbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
		     MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_OFFSET);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
}
#endif

#ifdef CONFIG_CMD_SATA
int setup_sata(void)
{
	struct iomuxc_base_regs *const iomuxc_regs
		= (struct iomuxc_base_regs *) IOMUXC_BASE_ADDR;
	int ret = enable_sata_clock();
	if (ret)
		return ret;

	clrsetbits_le32(&iomuxc_regs->gpr[13],
			IOMUXC_GPR13_SATA_MASK,
			IOMUXC_GPR13_SATA_PHY_8_RXEQ_3P0DB
			|IOMUXC_GPR13_SATA_PHY_7_SATA2M
			|IOMUXC_GPR13_SATA_SPEED_3G
			|(3<<IOMUXC_GPR13_SATA_PHY_6_SHIFT)
			|IOMUXC_GPR13_SATA_SATA_PHY_5_SS_DISABLED
			|IOMUXC_GPR13_SATA_SATA_PHY_4_ATTEN_9_16
			|IOMUXC_GPR13_SATA_PHY_3_TXBOOST_0P00_DB
			|IOMUXC_GPR13_SATA_PHY_2_TX_1P104V
			|IOMUXC_GPR13_SATA_PHY_1_SLOW);

	return 0;
}
#endif

int mx6_rgmii_rework(struct phy_device *phydev)
{
#if  !defined(CONFIG_SPL_BUILD)
	phy_write(phydev, MDIO_DEVAD_NONE, 0x9, 0x1c00);

	/* min rx data delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x2);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0x4);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x4002);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0);

    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x2);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0x5);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x4002);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0);

    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x2);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0x6);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x4002);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0);

    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x2);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0x8);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0d, 0x4002);
    	phy_write(phydev, MDIO_DEVAD_NONE, 0x0e, 0x3ff);
	/* max rx/tx clock delay, min rx/tx control delay */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0b, 0x8104);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0c, 0xf0f0);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x0b, 0x104);
#endif
	return 0;
}

#if  !defined(CONFIG_SPL_BUILD)
#if defined(CONFIG_VIDEO_IPUV3)
struct display_info_t {
	int	bus;
	int	addr;
	int	pixfmt;
	int	(*detect)(struct display_info_t const *dev);
	void	(*enable)(struct display_info_t const *dev);
	struct	fb_videomode mode;
};

static void disable_lvds(struct display_info_t const *dev)
{
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;

	int reg = readl(&iomux->gpr[2]);

	reg &= ~(IOMUXC_GPR2_LVDS_CH0_MODE_MASK |
		 IOMUXC_GPR2_LVDS_CH1_MODE_MASK);

	writel(reg, &iomux->gpr[2]);
}

static void do_enable_hdmi(struct display_info_t const *dev)
{
	disable_lvds(dev);
	imx_enable_hdmi_phy();
}

/*
mode "800x480-54"
    # D: 30.303 MHz, H: 27.852 kHz, V: 53.562 Hz
    geometry 800 480 800 480 16
    timings 33000 127 129 17 3 32 20
    rgba 5/11,6/5,5/0,0/0
endmode
	 "VAR-WVGA",     57, 800, 480, 28000, 28, 17, 13, 20, 20, 13,
	 "VAR-WVGA-CTW", 57, 800, 480, 33000, 127, 129, 17, 3, 32, 20,
*/
static struct display_info_t const displays[] = {{
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= NULL,
	.enable	= NULL,
	.mode	= {
		.name           = "VAR-WVGA",
		.refresh        = 57,
		.xres           = 800,
		.yres           = 480,
		.pixclock       = 15000,
		.left_margin    = 127,
		.right_margin   = 129,
		.upper_margin   = 17,
		.lower_margin   = 3,
		.hsync_len      = 32,
		.vsync_len      = 20,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} }, {
	.bus	= -1,
	.addr	= 0,
	.pixfmt	= IPU_PIX_FMT_RGB24,
	.detect	= NULL,
	.enable	= do_enable_hdmi,
	.mode	= {
		.name           = "HDMI",
		.refresh        = 60,
		.xres           = 640,
		.yres           = 480,
		.pixclock       = 39721,
		.left_margin    = 48,
		.right_margin   = 16,
		.upper_margin   = 33,
		.lower_margin   = 10,
		.hsync_len      = 96,
		.vsync_len      = 2,
		.sync           = 0,
		.vmode          = FB_VMODE_NONINTERLACED
} } };

int board_video_skip(void)
{
	int i;
	int ret;
	char const *panel = getenv("panel");

	if (!panel) {
			panel = displays[0].mode.name;
			printf("Panel default to %s\n", panel);
			i = 0;
	} else {
		for (i = 0; i < ARRAY_SIZE(displays); i++) {
			if (!strcmp(panel, displays[i].mode.name))
				break;
		}
	}

	if (i < ARRAY_SIZE(displays)) {
		ret = ipuv3_fb_init(&displays[i].mode, 0,
				    displays[i].pixfmt);
		if (!ret) {
			if (displays[i].enable)
				displays[i].enable(displays+i);
			printf("Display: %s (%ux%u)\n",
			       displays[i].mode.name,
			       displays[i].mode.xres,
			       displays[i].mode.yres);
		} else
			printf("LCD %s cannot be configured: %d\n",
			       displays[i].mode.name, ret);
	} else {
		printf("unsupported panel %s\n", panel);
		return -EINVAL;
	}

	return 0;
}

static void setup_iomux_backlight(void)
{
	MX6QDL_SET_PAD(PAD_DISP0_DAT9__GPIO_4_30 , MUX_PAD_CTRL(ENET_PAD_CTRL));

	gpio_direction_output(VAR_SOM_BACKLIGHT_EN , 1);
	gpio_set_value(VAR_SOM_BACKLIGHT_EN, 0);			// Turn off backlight tiil board init done.
}

static void setup_display(void)
{
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;
	struct iomuxc *iomux = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int reg;

	setup_iomux_backlight();
	enable_ipu_clock();
	imx_setup_hdmi();

	/* Turn on LDB_DI0 and LDB_DI1 clocks */
	reg = readl(&mxc_ccm->CCGR3);
	reg |= MXC_CCM_CCGR3_LDB_DI0_MASK | MXC_CCM_CCGR3_LDB_DI1_MASK;
	writel(reg, &mxc_ccm->CCGR3);

	/* Set LDB_DI0 and LDB_DI1 clk select to 3b'011 */
	reg = readl(&mxc_ccm->cs2cdr);
	reg &= ~(MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_MASK |
		 MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_MASK);
	reg |= (3 << MXC_CCM_CS2CDR_LDB_DI0_CLK_SEL_OFFSET) |
	       (3 << MXC_CCM_CS2CDR_LDB_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->cs2cdr);

	reg = readl(&mxc_ccm->cscmr2);
	reg |= MXC_CCM_CSCMR2_LDB_DI0_IPU_DIV | MXC_CCM_CSCMR2_LDB_DI1_IPU_DIV;
	writel(reg, &mxc_ccm->cscmr2);

	reg = readl(&mxc_ccm->chsccdr);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0 <<
		MXC_CCM_CHSCCDR_IPU1_DI0_CLK_SEL_OFFSET);
	reg |= (CHSCCDR_CLK_SEL_LDB_DI0 <<
		MXC_CCM_CHSCCDR_IPU1_DI1_CLK_SEL_OFFSET);
	writel(reg, &mxc_ccm->chsccdr);

	reg = IOMUXC_GPR2_DI1_VS_POLARITY_ACTIVE_LOW |
	      IOMUXC_GPR2_DI0_VS_POLARITY_ACTIVE_LOW |
	      IOMUXC_GPR2_BIT_MAPPING_CH1_SPWG |
	      IOMUXC_GPR2_DATA_WIDTH_CH1_18BIT |
	      IOMUXC_GPR2_BIT_MAPPING_CH0_SPWG |
	      IOMUXC_GPR2_DATA_WIDTH_CH0_18BIT |
	      IOMUXC_GPR2_LVDS_CH0_MODE_ENABLED_DI0 |
	      IOMUXC_GPR2_LVDS_CH1_MODE_DISABLED;
	writel(reg, &iomux->gpr[2]);

	reg = readl(&iomux->gpr[3]);
	reg &= ~(IOMUXC_GPR3_LVDS0_MUX_CTL_MASK |
		 IOMUXC_GPR3_HDMI_MUX_CTL_MASK);
	reg |= (IOMUXC_GPR3_MUX_SRC_IPU1_DI0 <<
		IOMUXC_GPR3_LVDS0_MUX_CTL_OFFSET) |
	       (IOMUXC_GPR3_MUX_SRC_IPU1_DI0 <<
		IOMUXC_GPR3_HDMI_MUX_CTL_OFFSET);
	writel(reg, &iomux->gpr[3]);
}
#endif /* CONFIG_VIDEO_IPUV3 */
#endif /* #if  !defined(CONFIG_SPL_BUILD) */

#ifdef CONFIG_I2C_MXC
void setup_local_i2c(void){
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info1));
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info2));
	setup_i2c(2, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c_pad_info3));
}
#endif

/*
 * Do not overwrite the console
 * Use always serial for U-Boot console
 */
int overwrite_console(void)
{
	return 1;
}
void enet_board_reset(int rst)
{

	MX6QDL_SET_PAD(PAD_ENET_CRS_DV__GPIO_1_25, MUX_PAD_CTRL(ENET_PAD_CTRL));

	gpio_direction_output(IMX_GPIO_NR(1,25) , 1);
	if (rst) {
		printf("1G RESET\n");
		gpio_set_value(IMX_GPIO_NR(1,25), 0);
	}
	else {
		printf("1G NORMAL\n");
		gpio_set_value(IMX_GPIO_NR(1,25), 1);
	}
}

int board_phy_config(struct phy_device *phydev)
{

#if  !defined(CONFIG_SPL_BUILD)
    /* min rx data delay */
	ksz9021_phy_extended_write(phydev,
			MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
	/* min tx data delay */
	ksz9021_phy_extended_write(phydev,
			MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
	/* max rx/tx clock delay, min rx/tx control */
	ksz9021_phy_extended_write(phydev,
			MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	mx6_rgmii_rework(phydev);
#endif
	return 0;
}
int board_eth_init(bd_t *bis)
{
#if  !defined(CONFIG_SPL_BUILD)
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret;

	setup_iomux_enet();

#ifdef CONFIG_FEC_MXC
	bus = fec_get_miibus(base, -1);

	if (!bus){
		printf("FEC MXC bus: %s:failed\n", __func__);
		return 0;
	}
	/* scan phy 4,5,6,7 */
	phydev = phy_find_by_mask(bus, (0xf << 7), PHY_INTERFACE_MODE_RGMII);
	if (!phydev) {
		printf("FEC MXC phy find: %s:failed\n", __func__);
		free(bus);
		return 0;
	}
	printf("using phy at %d\n", phydev->addr);

	ret  = fec_probe(bis, -1, base, bus, phydev);
	if (ret) {
		printf("FEC MXC probe: %s:failed\n", __func__);
		free(phydev);
		free(bus);
	}
#endif

	if (ret)
		printf("FEC MXC: %s:failed\n", __func__);

#endif /* #if  !defined(CONFIG_SPL_BUILD) */
	return 0;
}

char is_som_solo(void){
int oldbus;
char flag;

	oldbus = i2c_get_bus_num();
	i2c_set_bus_num(0);

	if (i2c_probe(0x8))
		flag = true;
	else
		flag = false;
	i2c_set_bus_num(oldbus);

	return flag;
}


static int var_som_rev(void)
{
	return 0;
}

u32 get_board_rev(void)
{
	int rev = var_som_rev();

	return (get_cpu_rev() & ~(0xF << 8)) | rev;
}

int board_early_init_f(void)
{
	var_setup_iomux_per_vcc_en();
	setup_iomux_uart();

#if  !defined(CONFIG_SPL_BUILD)
#if defined(CONFIG_VIDEO_IPUV3)
	setup_display();
#endif

#ifdef CONFIG_SYS_USE_SPINOR
	setup_spinor();
#endif

#ifdef CONFIG_SYS_USE_EIMNOR
	setup_eimnor();
#endif

#ifdef CONFIG_CMD_SATA
	setup_sata();
#endif

#ifdef CONFIG_SYS_USE_NAND
	setup_gpmi_nand();
#endif

#ifdef CONFIG_USB_EHCI_MX6
	setup_usb();
#endif

	gpio_direction_output(VAR_SOM_BACKLIGHT_EN , 1);
	gpio_set_value(VAR_SOM_BACKLIGHT_EN, 0);		// Turn off backlight.
#endif /* #if  !defined(CONFIG_SPL_BUILD) */

	//Reset CODEC to allow i2c communication
	codec_reset(1);
	udelay(1000 * 100);

	return 0;
}

#ifdef CONFIG_USB_EHCI_MX6
#define VSC_USB_H1_PWR_EN	IMX_GPIO_NR(4, 15)
#define VSC_USB_OTG_PWR_EN	IMX_GPIO_NR(3, 22)

static char is_solo_custom_board(void){
int oldbus;
char flag;

	oldbus = i2c_get_bus_num();
	i2c_set_bus_num(0);
	if (i2c_probe(0x51) == 0)
		flag = true;
	else
		flag = false;
	i2c_set_bus_num(oldbus);

	return flag;
}

static void setup_usb(void)
{
	struct iomuxc_base_regs *const iomuxc_regs
		= (struct iomuxc_base_regs *) IOMUXC_BASE_ADDR;
 
	/* config OTG ID iomux */
	MX6QDL_SET_PAD(PAD_GPIO_1__USB_OTG_ID, MUX_PAD_CTRL(USB_PAD_CTRL));
	clrsetbits_le32(&iomuxc_regs->gpr[1],
			IOMUXC_GPR1_OTG_ID_MASK,
			IOMUXC_GPR1_OTG_ID_GPIO1);

	if (is_solo_custom_board()) {
		MX6QDL_SET_PAD(PAD_KEY_ROW4__GPIO_4_15, MUX_PAD_CTRL(NO_PAD_CTRL));
		MX6QDL_SET_PAD(PAD_EIM_D22__GPIO_3_22, MUX_PAD_CTRL(NO_PAD_CTRL));
		gpio_direction_output(VSC_USB_H1_PWR_EN, 0);
		gpio_direction_output(VSC_USB_OTG_PWR_EN, 0);
	}
}

int board_ehci_hcd_init(int port)
{
	printf("%s(%d)\n", __func__, port);
	if (is_solo_custom_board()) {
		/* no hub to reset */
	} else {
		/* hub present but pulled up */
	}
	return 0;
}

int board_ehci_power(int port, int on)
{
	printf("%s(%d, %d)\n", __func__, port, on);
	if (is_solo_custom_board()) {
		if (port) {
			gpio_set_value(VSC_USB_H1_PWR_EN, on);
		} else {
			gpio_set_value(VSC_USB_OTG_PWR_EN, on);
		}
	} else {
		/* no power enable needed */
	}
	return 0;
}
#endif

int imx_pcie_reset(void);
/* PCI Probe function. */
void pci_init_board(void)
{
	/* At this point we only support reset. This is to solve the reboot hang problem due to wrong pcie status */
	imx_pcie_reset();
}

int board_init(void)
{
	int ret = 0;

	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	gd->bd->bi_arch_number = CONFIG_MACH_VAR_SOM_MX6;
	
	
#ifdef CONFIG_I2C_MXC
#if  !defined(CONFIG_SPL_BUILD)
	setup_local_i2c();
	if (!is_som_solo())
		ret = setup_pmic_voltages();
	if (ret)
		return -1;
#endif /* !defined(CONFIG_SPL_BUILD) */
#endif

	return ret;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"mmc0", MAKE_CFGVAL(0x40, 0x30, 0x00, 0x00)},
	{NULL,   0},
};
#endif



static void setup_variscite_touchscreen_type(void)
{
#if CONFIG_I2C_MXC
extern void i2c_set_base_address(u32 i2c_base_addr);
char buf[256];
char *bootargs;
char *mmcargs;
char *netargs;
char flag;
int oldbus;

	if (is_solo_custom_board())
		return;

	oldbus = i2c_get_bus_num();
	i2c_set_bus_num(2);
	if (i2c_probe(0x38) == 0)
		flag = true;
	else
		flag = false;
	i2c_set_bus_num(oldbus);


	if (flag)
		printf("Switching to alternate screen\n");
	else
		return;

	bootargs = getenv ("bootargs");
	if (NULL != strstr(bootargs, "screen_alternate=yes")) return;

	if (bootargs != NULL){
		strcpy (buf, bootargs);
		strcat (buf, " screen_alternate=yes");
		setenv ("bootargs", buf);
	}

	mmcargs = getenv ("mmcargs");
	if (NULL != strstr(mmcargs, "screen_alternate=yes")) return;
	if (mmcargs != NULL){
		strcpy (buf, mmcargs);
		strcat (buf, " screen_alternate=yes");
		setenv ("mmcargs", buf);
	}

	netargs = getenv ("netargs");
	if (NULL != strstr(netargs, "screen_alternate=yes")) return;
	if (netargs != NULL){
		strcpy (buf, netargs);
		strcat (buf, " screen_alternate=yes");
		setenv ("netargs", buf);
	}

#endif
}

int checkboard(void)
{
	char *s;

	printf("Board: Variscite VAR_SOM_MX6 ");


	if (is_mx6q()){
		if (is_cpu_pop_package()){
			printf ("Quad-POP\n");
		} else {
			printf ("Quad\n");
		}
		
	} else if (is_mx6d()){
		if (is_cpu_pop_package()){
			printf ("Dual-POP\n");
		} else {
			printf ("Dual\n");
		}
	} else if (is_mx6dl()) {
		if (is_som_solo())
			printf ("SOM-Dual\n");
		else
		   printf ("Dual Lite\n");
	} else if (is_mx6solo()){
		if (is_som_solo()){
			printf ("SOM-Solo\n");
		} else {
			printf ("Solo\n");
		}
	} else printf ("????\n");



	s = getenv ("var_auto_fdt_file");
	if (s[0] != 'Y') return 0;

	setenv("mmcroot" , "/dev/mmcblk0p2 rootwait rw");
	if (is_mx6q()){
		if (is_cpu_pop_package()){
			setenv("fdt_file", "imx6q-var-dart.dtb");
			if (6 == get_mmc_boot_device())
				setenv("mmcroot" , "/dev/mmcblk2p2 rootwait rw");
			else
				setenv("mmcroot" , "/dev/mmcblk1p2 rootwait rw");
		} else {
			if (is_solo_custom_board())
				setenv("fdt_file", "imx6q-var-som-vsc.dtb");
			else
				setenv("fdt_file", "imx6q-var-som.dtb");
		}
	} else if (is_mx6d()){
		if (is_cpu_pop_package()){
			setenv("fdt_file", "imx6q-var-dart.dtb");
			if (6 == get_mmc_boot_device())
				setenv("mmcroot" , "/dev/mmcblk2p2 rootwait rw");
			else
				setenv("mmcroot" , "/dev/mmcblk1p2 rootwait rw");
		} else {
			setenv("fdt_file", "imx6q-var-som.dtb");
		}
	}else if (is_mx6dl()) {
		if (is_som_solo()){
				if (is_solo_custom_board())
					setenv("fdt_file", "imx6dl-var-som-solo-vsc.dtb");
				else
					setenv("fdt_file", "imx6dl-var-som-solo.dtb");
		} else {
			setenv("fdt_file", "imx6dl-var-som.dtb");
		}
	} else if (is_mx6solo()){
		if (is_som_solo()){
			if (is_solo_custom_board())
				setenv("fdt_file", "imx6dl-var-som-solo-vsc.dtb");
			else
				setenv("fdt_file", "imx6dl-var-som-solo.dtb");
		} else {
			setenv("fdt_file", "imx6dl-var-som.dtb");
		}
	}

	setup_variscite_touchscreen_type();

	return 0;
}


int board_late_init(void)
{
#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif

	checkboard();
	gpio_direction_output(VAR_SOM_BACKLIGHT_EN , 1);
	gpio_set_value(VAR_SOM_BACKLIGHT_EN, 1);		// Turn off backlight.
//	if (is_som_solo())
//		var_setup_iomux_som_solo_usb_en();

	return 0;
}


