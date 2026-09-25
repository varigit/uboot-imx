/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 * Copyright 2018-2026 Variscite Ltd.
 */

#ifndef __IMX8MM_VAR_DART_H
#define __IMX8MM_VAR_DART_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x930000
#endif

#define CONFIG_SERIAL_TAG

/* ENET Config */
#if defined(CONFIG_FEC_MXC)
#define PHY_ANEG_TIMEOUT 20000
#endif

/* Link Definitions */
#define CFG_SYS_INIT_RAM_ADDR        0x40000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

/* DDR configs */
#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define DEFAULT_SDRAM_SIZE		(512 * SZ_1M) /* 512MB Minimum DDR4, see get_dram_size */
#define VAR_EEPROM_DRAM_START          (CONFIG_SYS_MEMTEST_START + \
					(DEFAULT_SDRAM_SIZE >> 1))

#define CFG_MXC_UART_BASE		UART1_BASE_ADDR
#define CFG_MXC_UART_BASE_2		UART4_BASE_ADDR

/* USDHC */
#define CFG_SYS_FSL_USDHC_NUM	2
#define CFG_SYS_FSL_ESDHC_ADDR	0

/* USB configs */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USBD_HS
#endif

#define CONFIG_USB_GADGET_VBUS_DRAW 2

#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)

/* Carrier board EEPROM */
#define CARRIER_EEPROM_BUS_SOM		0x02
#define CARRIER_EEPROM_BUS_DART		0x01
#define CARRIER_EEPROM_ADDR		0x54

/* Define the offset for the FDT FIT hash */
#define CFG_FIT_FDT_HASH_OFFSET 0x23800

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx8mm_var_dart_android.h"
#endif

#endif
