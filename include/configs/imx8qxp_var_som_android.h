/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * Copyright 2019-2020 Variscite Ltd.
 */

#ifndef IMX8QXP_VAR_SOM_ANDROID_H
#define IMX8QXP_VAR_SOM_ANDROID_H

#define CONFIG_USBD_HS

//#define CONFIG_BCB_SUPPORT
#define CONFIG_CMD_READ
#define CONFIG_USB_GADGET_VBUS_DRAW	2

#define FSL_FASTBOOT_FB_DEV "mmc"

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define HW_ENV_SETTINGS \
	"cmaargs=" \
		"if test $sdram_size = 2048; then " \
			"setenv cmavar 800M@0x960M-0xe00M; " \
		"else " \
			"setenv cmavar 1184M@0x960M-0xe00M; " \
		"fi; " \
		"setenv bootargs ${bootargs} " \
			"cma=${cmavar};\0"

#define BOOT_ENV_SETTINGS \
	"bootcmd=" \
		"run cmaargs; " \
		"boota ${fastboot_dev}\0"

#define CONFIG_EXTRA_ENV_SETTINGS					\
	HW_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	"splashpos=m,m\0"	  \
	"fdt_high=0xffffffffffffffff\0"	  \
	"initrd_high=0xffffffffffffffff\0" \
	"panel=NULL\0" \
	"bootargs=" \
		"console=ttyLP3,115200 " \
		"init=/init " \
		"androidboot.console=ttyLP3 " \
		"consoleblank=0 " \
		"androidboot.hardware=nxp " \
		"firmware_class.path=/vendor/firmware " \
		"loop.max_part=7 " \
		"androidboot.fbTileSupport=enable " \
		"androidboot.primary_display=imx-drm " \
		"androidboot.wificountrycode=CN " \
		"androidboot.vendor.sysrq=1 " \
		"transparent_hugepage=never\0"

#define BOOTLOADER_RBIDX_OFFSET  0x3FE000
#define BOOTLOADER_RBIDX_START   0x3FF000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0

#endif /* IMX8QXP_VAR_SOM_ANDROID_H */
