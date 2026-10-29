/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 NXP
 * Copyright 2023-2026 Variscite Ltd.
 */

#ifndef __IMX93_VAR_SOM_H
#define __IMX93_VAR_SOM_H

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512

#define CFG_SYS_INIT_RAM_ADDR	0x80000000
#define CFG_SYS_INIT_RAM_SIZE	0x200000

#define CFG_SYS_SDRAM_BASE	0x80000000
#define PHYS_SDRAM		0x80000000
#define DEFAULT_SDRAM_SIZE	(512 * SZ_1M) /* 512MB Minimum DDR4, see get_dram_size */
#define PHYS_SDRAM_SIZE		DEFAULT_SDRAM_SIZE /* 512MB DDR */
#define VAR_EEPROM_DRAM_START	(PHYS_SDRAM + (DEFAULT_SDRAM_SIZE >> 1))
#define VAR_SOM_EEPROM_I2C_NAME		"i2c@42530000"
#define VAR_CARRIER_EEPROM_I2C_NAME	"i2c@44340000"

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR		WDG3_BASE_ADDR

#ifdef CONFIG_IMX_MATTER_TRUSTY
#define NS_ARCH_ARM64	1
#endif

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx93_evk_android.h"
#endif

#endif
