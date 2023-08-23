/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8QXP_VAR_SOM_ANDROID_H
#define IMX8QXP_VAR_SOM_ANDROID_H

#define CONFIG_USBD_HS

//#define CONFIG_BCB_SUPPORT
#define CONFIG_CMD_READ
#define CONFIG_USB_GADGET_VBUS_DRAW	2

#define FSL_FASTBOOT_FB_DEV "mmc"

//#define IMX_LOAD_HDMI_FIMRWARE
#define IMX_HDMI_FIRMWARE_LOAD_ADDR 0x9c000000
//#define IMX_HDMI_FIRMWARE_LOAD_ADDR (CONFIG_SYS_SDRAM_BASE + SZ_64M)
#define IMX_HDMITX_FIRMWARE_SIZE 0x20000
#define IMX_HDMIRX_FIRMWARE_SIZE 0x20000

#define CONFIG_FASTBOOT_USB_DEV 1

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define CONFIG_SYS_SPL_PTE_RAM_BASE 0x801F8000
#define CONFIG_EXTRA_ENV_SETTINGS					\
	"splashpos=m,m\0"	  \
	"fdt_high=0xffffffffffffffff\0"	  \
	"initrd_high=0xffffffffffffffff\0" \
	"hdp_file=hdmitxfw.bin\0" \
	"hdprx_file=hdmirxfw.bin\0" \
	"panel=NULL\0" \
	"bootargs=" \
		"console=ttyLP0,115200 " \
		"init=/init " \
		"androidboot.console=ttyLP0 " \
		"androidboot.hardware=nxp " \
		"cma=928M@0x960M-0xfc0M " \
		"firmware_class.path=/vendor/firmware " \
		"loop.max_part=7 bootconfig " \
		"androidboot.fbTileSupport=enable " \
		"androidboot.primary_display=imx-drm " \
		"androidboot.wificountrycode=US " \
		"androidboot.vendor.sysrq=1 " \
		"transparent_hugepage=never\0"

#ifdef CONFIG_IMX_TRUSTY_OS
#define NS_ARCH_ARM64 1
#define KEYSLOT_HWPARTITION_ID   2
#define KEYSLOT_BLKS             0x3FFF
#define AVB_RPMB

#define BOOTLOADER_RBIDX_OFFSET  0x3FE000
#define BOOTLOADER_RBIDX_START   0x3FF000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0
#endif
#undef CONFIG_SYS_SPL_MALLOC_START
#define CONFIG_SYS_SPL_MALLOC_START    0x82200000
#undef CONFIG_SYS_SPL_MALLOC_SIZE
#define CONFIG_SYS_SPL_MALLOC_SIZE     0x100000        /* 1 MB */
#endif /* IMX8QXP_VAR_SOM_ANDROID_H */
