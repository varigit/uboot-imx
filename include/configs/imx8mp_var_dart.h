/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 NXP
 * Copyright 2020-2026 Variscite Ltd.
 */

#ifndef __IMX8MP_VAR_DART_H
#define __IMX8MP_VAR_DART_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#define IMX_BOOT_IMAGE_GUID \
	EFI_GUID(0x928b33bc, 0xe58b, 0x4247, 0x9f, 0x1d, \
		 0x3b, 0xf1, 0xee, 0x1c, 0xda, 0xff)

#define CFG_SYS_UBOOT_BASE \
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* ENET Config */
#if defined(CONFIG_CMD_NET)
#define PHY_ANEG_TIMEOUT		20000
#endif

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR		0x40000000
#define CFG_SYS_INIT_RAM_SIZE		0x80000

/* DDR configs */
#define CFG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000
#define PHYS_SDRAM_SIZE			0xC0000000
#define DEFAULT_SDRAM_SIZE		(512 * SZ_1M)

#define CFG_MXC_UART_BASE		UART1_BASE_ADDR

#define CFG_SYS_FSL_USDHC_NUM	2
#define CFG_SYS_FSL_ESDHC_ADDR	0

/* EEPROM configs */
#define VAR_EEPROM_DRAM_START	(PHYS_SDRAM + (DEFAULT_SDRAM_SIZE >> 1))

/* Define the offset for the FDT FIT hash */
#define CFG_FIT_FDT_HASH_OFFSET 0x12000

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx8mp_var_dart_android.h"
#endif

#endif
