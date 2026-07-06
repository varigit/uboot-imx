/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 NXP
 * Copyright 2024-2026 Variscite Ltd.
 */

#ifndef __IMX95_VAR_DART_H
#define __IMX95_VAR_DART_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "env/nxp/imx_env.h"

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x90000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x90000000
#define PHYS_SDRAM                      0x90000000
/* Totally 8GB */
#define PHYS_SDRAM_SIZE			0x70000000UL /* 2GB  - 256MB DDR */
#define PHYS_SDRAM_2_SIZE 		0x180000000 /* 6GB */

#define DEFAULT_SDRAM_SIZE		(4UL * SZ_1G) /* 4GB Minimum DDR5, see get_dram_size */

#define CFG_SYS_FSL_USDHC_NUM	2

#define CFG_SYS_SECURE_SDRAM_BASE	0x8A000000 /* Secure DDR region for A55, SPL could use first 2MB */
#define CFG_SYS_SECURE_SDRAM_SIZE	0x06000000

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

/* USB configs */
#if defined(CONFIG_CMD_NET)
#define PHY_ANEG_TIMEOUT 20000
/* Number of Rx BD rings: 8 per ENETC instance */
#endif

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx95_var_dart_android.h"
#endif

#endif
