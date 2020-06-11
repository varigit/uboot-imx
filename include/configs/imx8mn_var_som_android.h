/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8MN_VAR_SOM_ANDROID_H
#define IMX8MN_VAR_SOM_ANDROID_H

#define CONFIG_USBD_HS

#define CONFIG_BCB_SUPPORT
#define CONFIG_CMD_READ
#define CONFIG_USB_GADGET_VBUS_DRAW	2

#define CONFIG_ANDROID_AB_SUPPORT
#define CONFIG_AVB_SUPPORT
#define CONFIG_SUPPORT_EMMC_RPMB
#ifdef CONFIG_ANDROID_AB_SUPPORT
#define CONFIG_SYSTEM_RAMDISK_SUPPORT
#endif
#define CONFIG_AVB_FUSE_BANK_SIZEW 0
#define CONFIG_AVB_FUSE_BANK_START 0
#define CONFIG_AVB_FUSE_BANK_END 0
#define CONFIG_FASTBOOT_LOCK
#define FSL_FASTBOOT_FB_DEV "mmc"

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN           (76 * SZ_1M)
#endif
#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_CMD_FASTBOOT
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_FASTBOOT_STORAGE_MMC
#define CONFIG_ANDROID_RECOVERY

#if defined CONFIG_SYS_BOOT_SATA
#define CONFIG_FASTBOOT_STORAGE_SATA
#define CONFIG_FASTBOOT_SATA_NO 0
#endif

#define CONFIG_CMD_BOOTA
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_SERIAL_TAG

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND
#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#define CMA_1G_ARGS "cma=320M@0x400M-0xb80M"
#define CMA_2G_ARGS "800M@0x400M-0xb80M"

#define HW_ENV_SETTINGS \
	"cmaargs=" \
		"if test $sdram_size = 1024; then " \
			"setenv cmavar 320M@0x400M-0xb80M; " \
		"else " \
			"setenv cmavar 800M@0x400M-0xb80M; " \
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
		"console=ttymxc3,115200 " \
		"earlycon=ec_imx6q,0x30a60000,115200 " \
		"init=/init " \
		"androidboot.console=ttymxc3 " \
		"consoleblank=0 " \
		"androidboot.hardware=freescale " \
		"firmware_class.path=/vendor/firmware " \
		"loop.max_part=7 " \
		"androidboot.primary_display=imx-drm " \
		"androidboot.wificountrycode=US " \
		"galcore.contiguousSize=33554432 "\
		"transparent_hugepage=never\0"

#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#endif /* IMX8MN_VAR_SOM_ANDROID_H */
