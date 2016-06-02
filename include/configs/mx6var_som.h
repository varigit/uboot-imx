/*
 * Copyright (C) 2012-2015 Freescale Semiconductor, Inc.
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * Configuration settings for Variscite VAR-SOM-MX6 board family.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __MX6VAR_SOM_CONFIG_H
#define __MX6VAR_SOM_CONFIG_H

#include <asm/arch/imx-regs.h>
#include <asm/imx-common/gpio.h>

#ifdef CONFIG_SPL
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_SPL_NAND_SUPPORT
#else
#define CONFIG_SPL_MMC_SUPPORT
#endif
#include "mx6var_spl.h"
#endif

/* Address of reserved 4Bytes in OCRAM for sending RAM size from SPL to U-Boot */
#define RAM_SIZE_ADDR	((CONFIG_SPL_TEXT_BASE) + (CONFIG_SPL_MAX_SIZE))

#define CONFIG_MX6

#ifdef CONFIG_MX6SOLO
#define CONFIG_MX6DL
#endif

/* Uncomment for PLUGIN mode support */
/* #define CONFIG_USE_PLUGIN */

/* Uncomment for SECURE mode support */
/* #define CONFIG_SECURE_BOOT */

#ifdef CONFIG_SECURE_BOOT
#ifndef CONFIG_CSF_SIZE
#define CONFIG_CSF_SIZE 0x4000
#endif
#endif

#include "mx6_common.h"
#include <linux/sizes.h>

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

#define CONFIG_SYS_GENERIC_BOARD

#define CONFIG_MACH_VAR_SOM_MX6		4419
#define CONFIG_MACH_TYPE		CONFIG_MACH_VAR_SOM_MX6

#define CONFIG_MXC_UART_BASE		UART1_BASE
#define CONFIG_CONSOLE_DEV		"ttymxc0"
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(16 * SZ_1M)

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_MXC_GPIO
#define CONFIG_CMD_GPIO

#define CONFIG_MXC_UART

#define CONFIG_CMD_FUSE
#ifdef CONFIG_CMD_FUSE
#define CONFIG_MXC_OCOTP
#endif

#define LOW_POWER_MODE_ENABLE

/* MMC Configs */
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	0
#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		0
#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_CMD_FAT
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_DOS_PARTITION

/*#define CONFIG_SUPPORT_EMMC_BOOT*/ /* eMMC specific */

/* Ethernet Configs */
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_XCV_TYPE		RGMII
#define CONFIG_ETHPRIME			"FEC"
#define CONFIG_FEC_MXC_PHYADDR		7
#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX		1
#define CONFIG_BAUDRATE			115200

/* Command definition */
#include <config_cmd_default.h>

/*#define CONFIG_CMD_BMODE*/
#define CONFIG_CMD_BOOTZ
#define CONFIG_CMD_SETEXPR
#undef CONFIG_CMD_IMLS

#define CONFIG_BOOTDELAY		1

#define CONFIG_LOADADDR			0x12000000
#define CONFIG_SYS_TEXT_BASE		0x17800000

#ifdef CONFIG_SYS_BOOT_NAND
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:16m(boot),16m(kernel),16m(dtb),-(rootfs) "
#else
#define MFG_NAND_PARTITION ""
#endif

#ifdef CONFIG_SYS_BOOT_NAND
#define MMC_DEV_SET " "
#else
#define MMC_DEV_SET "mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV)
#endif

#define MFG_ENV_SETTINGS \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"g_mass_storage.stall=0 g_mass_storage.removable=1 " \
		"g_mass_storage.idVendor=0x066F g_mass_storage.idProduct=0x37FF "\
		"g_mass_storage.iSerialNumber=\"\" "\
		"enable_wait_mode=off "\
		MFG_NAND_PARTITION \
		"\0" \
	"initrd_addr=0x12C00000\0" \
	"initrd_high=0xffffffff\0" \
	"bootcmd_mfg=run mfgtool_args;bootm ${loadaddr} ${initrd_addr} ${fdt_addr};\0" \


#define MMC_BOOT_ENV_SETTINGS \
	"uimage=uImage\0" \
	"fdt_file=" CONFIG_DEFAULT_FDT_FILE "\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	MMC_DEV_SET \
	"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} root=${mmcroot}; " \
		"run videoargs\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loaduimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${uimage}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootm ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootm; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootm; " \
		"fi;\0" \


#define NAND_BOOT_ENV_SETTINGS \
	"bootargs_ubifs=setenv bootargs console=${console},${baudrate} ubi.mtd=3 " \
		"root=ubi0:rootfs rootfstype=ubifs; " \
		"run videoargs\0" \
	"bootargs_emmc=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/mmcblk1p1 rootwait rw; " \
		"run videoargs\0" \
	"bootcmd=" \
		"if test ${chosen_rootfs} != emmc; then " \
			"run bootargs_ubifs; " \
		"else " \
			"run bootargs_emmc; " \
		"fi; " \
		"nand read ${loadaddr} 0x400000 0x600000;" \
		"nand read ${fdt_addr} 0x3e0000 0x20000;" \
		"bootm ${loadaddr} - ${fdt_addr}\0" \
	"mtdids=" MTDIDS_DEFAULT "\0" \
	"mtdparts=" MTDPARTS_DEFAULT "\0" \


#ifdef CONFIG_SYS_BOOT_NAND
#define BOOT_ENV_SETTINGS	NAND_BOOT_ENV_SETTINGS
#else
#define BOOT_ENV_SETTINGS	MMC_BOOT_ENV_SETTINGS
#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev};" \
	"if mmc rescan; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loaduimage; then " \
				"run mmcboot; " \
			"else " \
				"run netboot; " \
			"fi; " \
		"fi; " \
	"else run netboot; fi"
#endif


#define CONFIG_EXTRA_ENV_SETTINGS \
	MFG_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	"var_auto_env=Y\0" \
	"fdt_addr=0x18000000\0" \
	"fdt_high=0xffffffff\0" \
	"splash_filename=/boot/splash.bmp\0" \
	"splashimage=0x18100000\0" \
	"enable_splash=setenv splash_filename /boot/splash.bmp; " \
		"setenv splashimage 0x18100000\0" \
	"disable_splash=setenv splash_filename; setenv splashimage\0" \
	"console=" CONFIG_CONSOLE_DEV "\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs rw " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp; " \
		"run videoargs\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${uimage}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"bootm ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootm; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootm; " \
		"fi;\0" \
	"videoargs=" \
		"if hdmidet; then " \
			"setenv bootargs ${bootargs} " \
				"video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24; " \
		"fi; " \
		"setenv bootargs ${bootargs} " \
			"video=mxcfb1:off; " \
		"i2c dev 2; " \
		"if i2c probe 0x38; then " \
			"setenv bootargs ${bootargs} " \
				"screen_alternate=yes;" \
		"fi;\0"


#define CONFIG_ARP_TIMEOUT		200UL

/* Miscellaneous configurable options */
#undef CONFIG_SYS_PROMPT
#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_PROMPT		"VAR-SOM-MX6 (NAND) => "
#else
#define CONFIG_SYS_PROMPT		"VAR-SOM-MX6 (MMC) => "
#endif
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE		1024

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		256
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE

#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x10000000
#define CONFIG_SYS_MEMTEST_END		0x10010000
#define CONFIG_SYS_MEMTEST_SCRATCH	0x10800000

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_CMDLINE_EDITING
#define CONFIG_STACKSIZE		(128 * 1024)

/* Physical Memory Map */
#define CONFIG_NR_DRAM_BANKS		1
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

/* FLASH and environment organization */
#define CONFIG_SYS_NO_FLASH

#define CONFIG_ENV_SIZE			(8 * 1024)

#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
/* NAND boot config */
#define CONFIG_SYS_NAND_U_BOOT_START	CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_NAND_U_BOOT_OFFS	0x200000
#else
#define CONFIG_ENV_IS_IN_MMC
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

#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_CMD_NAND
#define CONFIG_CMD_NAND_TRIMFFS

/* UBI/UBIFS support */
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_RBTREE
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_LZO

#define MTDIDS_DEFAULT		"nand0=nandflash-0"

/*
 * Partions' layout for NAND is:
 *     mtd0: 2M       (spl) First boot loader
 *     mtd1: 2M       (u-boot, dtb)
 *     mtd2: 6M       (kernel)
 *     mtd4: left     (rootfs)
 */
/* Default mtd partition table */
#define MTDPARTS_DEFAULT	"mtdparts=nandflash-0:"\
					"2m(spl),"\
					"2m(u-boot),"\
					"6m(kernel),"\
					"-(rootfs)"	/* ubifs */

/* NAND stuff */
#define CONFIG_NAND_MXS
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION

/* DMA stuff, needed for GPMI/MXS NAND support */
#define CONFIG_APBH_DMA
#define CONFIG_APBH_DMA_BURST
#define CONFIG_APBH_DMA_BURST8
#endif

#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(0x3E0000)
#elif defined(CONFIG_ENV_IS_IN_NAND)
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET		(0x180000)
#define CONFIG_ENV_SECT_SIZE		(128 << 10)
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#endif

#define CONFIG_OF_LIBFDT

#ifndef CONFIG_SYS_DCACHE_OFF
#define CONFIG_CMD_CACHE
#endif

/* Framebuffer */
#define CONFIG_VIDEO
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_IPUV3
#define CONFIG_CFB_CONSOLE
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_BMP_LOGO
#ifdef CONFIG_MX6DL
#define CONFIG_IPUV3_CLK 198000000
#else
#define CONFIG_IPUV3_CLK 264000000
#endif
#define CONFIG_IMX_HDMI
#define CONFIG_CMD_HDMIDETECT
#define CONFIG_IMX_VIDEO_SKIP
#define CONFIG_CMD_BMP
#endif

#define PMIC_I2C_BUS		1
#define MX6CB_CDISPLAY_I2C_BUS	2
#define MX6CB_CDISPLAY_I2C_ADDR	0x38

/* I2C Configs */
#define CONFIG_CMD_I2C
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_SPEED		100000

/* PMIC */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE100
#define CONFIG_POWER_PFUZE100_I2C_ADDR	0x08

/* USB Configs */
#define CONFIG_CMD_USB
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS		0
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2 /* Enabled USB controller number */
#endif

#define CONFIG_CI_UDC
#define CONFIG_USBD_HS
#define CONFIG_USB_GADGET_DUALSPEED

/* Uncomment for USB Ethernet Gadget support */
/*
 * #define CONFIG_USB_ETHER
 * #define CONFIG_USB_ETH_CDC
 */
#define CONFIG_NETCONSOLE

#define CONFIG_USB_GADGET
#define CONFIG_CMD_USB_MASS_STORAGE
#define CONFIG_USB_GADGET_MASS_STORAGE
#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_USB_GADGET_VBUS_DRAW	2

#define CONFIG_G_DNL_VENDOR_NUM		0x18d1
#define CONFIG_G_DNL_PRODUCT_NUM	0x0d02
#define CONFIG_G_DNL_MANUFACTURER	"Variscite"

#if defined(CONFIG_ANDROID_SUPPORT)
#include "mx6var_som_android.h"
#endif

/* USB Device Firmware Update support */
#define CONFIG_CMD_DFU
#define CONFIG_DFU_FUNCTION
#define CONFIG_DFU_MMC
#define CONFIG_DFU_RAM
#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_DFU_NAND
#endif
#if 0
#define CONFIG_SYS_DFU_DATA_BUF_SIZE SZ_32M
#define DFU_DEFAULT_POLL_TIMEOUT 300
#endif

#endif	/* __MX6VAR_SOM_CONFIG_H */
