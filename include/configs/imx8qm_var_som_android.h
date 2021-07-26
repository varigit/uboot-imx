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

#define CONFIG_ANDROID_AB_SUPPORT
#ifdef CONFIG_ANDROID_AB_SUPPORT
#define CONFIG_SYSTEM_RAMDISK_SUPPORT
#endif
#define FSL_FASTBOOT_FB_DEV "mmc"

#define IMX_LOAD_HDMI_FIMRWARE
//#define IMX_HDMI_FIRMWARE_LOAD_ADDR (CONFIG_SYS_SDRAM_BASE + SZ_64M)
#define IMX_HDMI_FIRMWARE_LOAD_ADDR 0x9c000000
#define IMX_HDMITX_FIRMWARE_SIZE 0x20000
#define IMX_HDMIRX_FIRMWARE_SIZE 0x20000

#define CONFIG_FASTBOOT_USB_DEV 1

#undef CONFIG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

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
		"consoleblank=0 " \
		"androidboot.hardware=nxp " \
		"cma=1184M@0x960M-0xe00M " \
		"firmware_class.path=/vendor/firmware " \
		"loop.max_part=7 " \
		"androidboot.fbTileSupport=enable " \
		"androidboot.primary_display=imx-drm " \
		"androidboot.wificountrycode=CN " \
		"androidboot.vendor.sysrq=1 " \
		"transparent_hugepage=never\0"

#ifdef CONFIG_IMX_TRUSTY_OS
#define NS_ARCH_ARM64 1
#define KEYSLOT_HWPARTITION_ID   2
#define KEYSLOT_BLKS             0x3FFF
#define AVB_RPMB

#ifdef CONFIG_SPL_BUILD
#undef CONFIG_BLK
#define CONFIG_FSL_CAAM_KB
#define CONFIG_SPL_CRYPTO_SUPPORT
#define CONFIG_SYS_FSL_SEC_LE
#endif
#endif

#define AVB_AB_I_UNDERSTAND_LIBAVB_AB_IS_DEPRECATED

#endif /* IMX8QXP_VAR_SOM_ANDROID_H */
