/*
 * Copyright (C) 2012-2013 Freescale Semiconductor, Inc.
 *
 * Copyright (C) 2014 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite VAR_SOM_MX6 board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __MX6Q_VAR_SOM_CONFIG_H
#define __MX6Q_VAR_SOM_CONFIG_H

#define CONFIG_MACH_VAR_SOM_MX6 	4419
#define CONFIG_MACH_TYPE		CONFIG_MACH_VAR_SOM_MX6

#define CONFIG_MXC_UART_BASE		UART1_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc0"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"
#define VAR_VERSION_STRING		"Variscite VAR-SOM-MX6 UB V0.5"

/* USB Configs */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET       /* For OTG port */
#define CONFIG_MXC_USB_PORTSC	(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS	0
/*#define MX6_FORCE_OTG_HOST	1 */	/* Enable if you want to force the OTG to be HOST always */

#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_PROMPT		"VAR_SOM_MX6(nand) U-Boot > "
#else
#define CONFIG_SYS_PROMPT		"VAR_SOM_MX6(sd) U-Boot > "
#endif

#define CONFIG_CMD_PCI
#ifdef CONFIG_CMD_PCI
#define CONFIG_PCI
/*#define CONFIG_PCI_PNP
#define CONFIG_PCI_SCAN_SHOW */
#define CONFIG_PCIE_IMX
#endif


#if defined(CONFIG_MX6DL)
#define CONFIG_DEFAULT_FDT_FILE		"imx6dl-var-som.dtb"
#elif defined(CONFIG_MX6S)
#define CONFIG_DEFAULT_FDT_FILE		"imx6dl-var-som.dtb"
#elif defined(CONFIG_MX6Q)
#define CONFIG_DEFAULT_FDT_FILE		"imx6q-var-som.dtb"
#else
#define CONFIG_DEFAULT_FDT_FILE		"imx6q-var-som.dtb"
#endif


#include "mx6var_common.h"
#include <asm/imx-common/gpio.h>

#if defined CONFIG_SYS_BOOT_SPINOR
#error"VAR-SOM-MX6 don't use SPINOR"
#elif defined CONFIG_SYS_BOOT_EIMNOR
#error"VAR-SOM-MX6 don't use EIMNOR"
#elif defined CONFIG_SYS_BOOT_SATA
#error"VAR-SOM-MX6 don't use BOOT SATA"
#endif

#endif       /* __MX6Q_VAR_SOM_CONFIG_H */
