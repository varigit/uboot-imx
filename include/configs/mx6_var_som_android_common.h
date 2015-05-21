/*
 * Copyright (C) 2013-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef MX6_VAR_SOM_ANDROID_COMMON_H
#define MX6_VAR_SOM_ANDROID_COMMON_H

#define CONFIG_SERIAL_TAG

#define CONFIG_USB_DEVICE
#define CONFIG_IMX_UDC		       1

#define CONFIG_FASTBOOT		       1
#define CONFIG_FASTBOOT_VENDOR_ID      		0x18d1
#define CONFIG_FASTBOOT_PRODUCT_ID     		0x0d02
#define CONFIG_FASTBOOT_BCD_DEVICE     		0x311
#define CONFIG_FASTBOOT_MANUFACTURER_STR	"Variscite"
#define CONFIG_FASTBOOT_PRODUCT_NAME_STR	"i.mx6 VAR-MX6 System"
#define CONFIG_FASTBOOT_INTERFACE_STR		"Android fastboot"
#define CONFIG_FASTBOOT_CONFIGURATION_STR	"Android fastboot"
#define CONFIG_FASTBOOT_SERIAL_NUM		"12345"
#define CONFIG_FASTBOOT_SATA_NO		 	0

#if 0
#if defined CONFIG_SYS_BOOT_NAND
#define CONFIG_FASTBOOT_STORAGE_NAND
#else
#define CONFIG_FASTBOOT_STORAGE_MMC
#endif
#endif
/* Variscite Lollipop support booti from MMC only (sdcard or eMMC) */
#define CONFIG_FASTBOOT_STORAGE_MMC

/*  For system.img growing up more than 256MB, more buffer needs
*   to receive the system.img*/
#define CONFIG_FASTBOOT_TRANSFER_BUF	0x2c000000
#define CONFIG_FASTBOOT_TRANSFER_BUF_SIZE 0x19000000 /* 400M byte */


#define CONFIG_CMD_BOOTI
#define CONFIG_ANDROID_RECOVERY
/* which mmc bus is your main storage ? */
#define CONFIG_ANDROID_MAIN_MMC_BUS 2
#define CONFIG_ANDROID_BOOT_PARTITION_MMC 1
#define CONFIG_ANDROID_SYSTEM_PARTITION_MMC 5
#define CONFIG_ANDROID_RECOVERY_PARTITION_MMC 2
#define CONFIG_ANDROID_CACHE_PARTITION_MMC 6

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_EXTRA_ENV_SETTINGS					\
	"splashpos=m,m\0"	  \
	"fdt_high=0xffffffff\0"	  \
	"initrd_high=0xffffffff\0" \

#if defined(CONFIG_FASTBOOT_STORAGE_NAND)
#define ANDROID_FASTBOOT_NAND_PARTS "2m@2m(uboot) 16m@4m(boot) 28m@20m(recovery) 464m@48m(android_root)ubifs"
#endif

#endif /* MX6_VAR_SOM_ANDROID_COMMON_H */
