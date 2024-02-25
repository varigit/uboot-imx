/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8MN_VAR_SOM_ANDROID_H
#define IMX8MN_VAR_SOM_ANDROID_H

#define FSL_FASTBOOT_FB_DEV "mmc"

#undef CFG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#ifdef CONFIG_SPL_BUILD
#define CFG_MALLOC_F_ADDR 0x970000
#endif

#define HW_ENV_SETTINGS \
	"cmaargs=" \
		"if test $sdram_size = 1024; then " \
			"setenv cmavar 320M@0x400M-0xb80M; " \
			"setenv galcore_var 'galcore.contiguousSize=33554432'; " \
		"else " \
			"setenv cmavar 800M@0x400M-0xb80M; " \
		"fi; " \
		"setenv bootargs ${bootargs} " \
			"cma=${cmavar} ${galcore_var}; \0"

#define BOOT_ENV_SETTINGS \
	"bootcmd=" \
		"run cmaargs; " \
		"boota ${fastboot_dev}\0"

#define CFG_EXTRA_ENV_SETTINGS					\
	HW_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	"splashpos=m,m\0"	  \
	"fdt_high=0xffffffffffffffff\0"	  \
	"initrd_high=0xffffffffffffffff\0" \
	"panel=NULL\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \
	"bootargs=" \
		"console=ttymxc3,115200 " \
		"earlycon=ec_imx6q,0x30a60000,115200 " \
		"init=/init " \
		"firmware_class.path=/vendor/firmware " \
		"fw_devlink.strict=0 " \
		"bootconfig " \
		"loop.max_part=7 " \
		"bootconfig " \
		"transparent_hugepage=never " \
		"androidboot.console=ttymxc3 " \
		"androidboot.hardware=nxp " \
		"androidboot.wificountrycode=US " \
		"androidboot.vendor.sysrq=1\0"

/* Enable mcu firmware flash */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
#define ANDROID_MCU_FRIMWARE_DEV_TYPE DEV_MMC
#define ANDROID_MCU_FIRMWARE_START 0x500000
#define ANDROID_MCU_OS_PARTITION_SIZE 0x40000
#define ANDROID_MCU_FIRMWARE_SIZE  0x20000
#define ANDROID_MCU_FIRMWARE_HEADER_STACK 0x20020000
#endif

#define CFG_SYS_SPL_PTE_RAM_BASE    0x41580000

#ifdef CONFIG_IMX_TRUSTY_OS
#define BOOTLOADER_RBIDX_OFFSET  0x3FE000
#define BOOTLOADER_RBIDX_START   0x3FF000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#define AVB_RPMB
#define KEYSLOT_HWPARTITION_ID 2
#define KEYSLOT_BLKS             0x1FFF
#define NS_ARCH_ARM64 1
#endif

#endif /* IMX8MN_VAR_SOM_ANDROID_H */
