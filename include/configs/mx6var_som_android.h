/*
 * Copyright (C) 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX6VAR_SOM_ANDROID_H
#define __MX6VAR_SOM_ANDROID_H

#define CONFIG_CMD_FASTBOOT
#define CONFIG_CMD_READ
#define CONFIG_BCB_SUPPORT
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_FASTBOOT_FLASH

/* For NAND we don't support lock/unlock */
#ifndef CONFIG_SYS_BOOT_NAND
/*#define CONFIG_FASTBOOT_LOCK*/
#endif

#define FSL_FASTBOOT_FB_DEV "mmc"
#define FSL_FASTBOOT_DATA_PART_NUM 4
#define FSL_FASTBOOT_FB_PART_NUM 11
#define FSL_FASTBOOT_PR_DATA_PART_NUM 12

#define CONFIG_FSL_CAAM_KB
#define CONFIG_CMD_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#define CONFIG_FSL_FASTBOOT
#define CONFIG_ANDROID_RECOVERY

#define CONFIG_FASTBOOT_STORAGE_MMC

#define CONFIG_ANDROID_MAIN_MMC_BUS 2
#define CONFIG_ANDROID_BOOT_PARTITION_MMC 1
#define CONFIG_ANDROID_SYSTEM_PARTITION_MMC 5
#define CONFIG_ANDROID_RECOVERY_PARTITION_MMC 2
#define CONFIG_ANDROID_CACHE_PARTITION_MMC 6
#define CONFIG_ANDROID_DATA_PARTITION_MMC 4
#define CONFIG_ANDROID_MISC_PARTITION_MMC 8

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
#define ANDROID_FASTBOOT_NAND_PARTS "16m@64m(boot) 16m@80m(recovery) 1m@96m(misc) 810m@97m(android_root)ubifs"
#endif

#define CONFIG_CMD_BOOTA
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_SERIAL_TAG
#define CONFIG_RESET_CAUSE

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND
#undef BOOT_ENV_SETTINGS


#define BOOT_ENV_SETTINGS \
	"bootcmd=" \
		"run videoargs; " \
		"boota ${boota_dev}\0" \
	"bootcmd_android_recovery=" \
		"run videoargs; " \
		"boota ${recovery_dev} recovery\0" \
	"fastboot_dev=mmc1\0" \
	"boota_dev=mmc1\0" \
	"recovery_dev=mmc1\0" \
	"dev_autodetect=yes\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	VIDEO_ENV_SETTINGS \
	"splashpos=m,m\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"bootargs=" \
		"console=ttymxc0,115200 " \
		"init=/init " \
		"vmalloc=128M " \
		"androidboot.console=ttymxc0 " \
		"consoleblank=0 " \
		"androidboot.hardware=freescale " \
		"cma=448M " \
		"firmware_class.path=/system/etc/firmware\0"


#define CONFIG_USB_FASTBOOT_BUF_ADDR   CONFIG_SYS_LOAD_ADDR
#ifdef CONFIG_FASTBOOT_STORAGE_NAND
#define CONFIG_USB_FASTBOOT_BUF_SIZE   0x32000000
#else
#define CONFIG_USB_FASTBOOT_BUF_SIZE   0x19000000
#endif

#endif	/* __MX6VAR_SOM_ANDROID_H */
