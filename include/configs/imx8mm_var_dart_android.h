/*
 * Copyright 2020 NXP
 * Copyright 2021 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX8MM_VAR_DART_ANDROID_H
#define __MX8MM_VAR_DART_ANDROID_H


#define FSL_FASTBOOT_FB_DEV "mmc"

#define CONFIG_SERIAL_TAG

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define KERNEL_ENV_SETTINGS \
	"kernelargs=" \

#define ANDROID_ENV_SETTINGS \
	"kernelbootargs=" \
		"if test ${board_name} = VAR-SOM-MX8M-MINI; then " \
			"setenv console ${console}; " \
			"setenv hardware_variant ${board_name}; " \
			"setenv androidconsole ttymxc3; " \
			"setenv som_ver ${som_rev}; " \
			"if test ${som_rev} -ge 2; then " \
				"setenv wifi_args 'moal.mod_para=wifi_mod_para_iw612.conf'; " \
			"fi; " \
		"else " \
			"setenv console ${console}; " \
			"setenv hardware_variant ${board_name}; " \
			"setenv androidconsole ttymxc0; " \
			"setenv som_ver ${som_rev}; " \
			"if test ${som_rev} -ge 2; then " \
				"setenv wifi_args 'moal.mod_para=wifi_mod_para_iw612.conf'; " \
			"fi; " \
		"fi; " \
		"setenv bootargs console=${console} ${bootargs} " \
				"${wifi_args} androidboot.console=${androidconsole} " \
				"androidboot.hardware_variant=${hardware_variant} " \
				"androidboot.som_ver=${som_ver}; \0"


#define BOOT_ENV_SETTINGS \
	"bootcmd=" \
		"run kernelbootargs; " \
		"bootmcu; boota ${fastboot_dev}\0"

#define CFG_EXTRA_ENV_SETTINGS		\
	ANDROID_ENV_SETTINGS  \
	BOOT_ENV_SETTINGS \
	"splashpos=m,m\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\
	"console=ttymxc0,115200\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \
	"bootargs=" \
		"init=/init " \
		"cma=800M@0x400M-0xb80M " \
		"firmware_class.path=/vendor/firmware " \
		"loop.max_part=7 " \
		"bootconfig " \
		"androidboot.vendor.sysrq=1 " \
		"transparent_hugepage=never " \
		"androidboot.lcd_density=160 " \
		"androidboot.hardware=nxp\0"

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

#endif /*__MX8MM_VAR_DART_ANDROID_H */