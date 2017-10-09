/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Copyright (C) 2015 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite  i.MX6UL DART board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MX6UL_VAR_DART_H
#define __MX6UL_VAR_DART_H

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>
#include "mx6_common.h"
#include <asm/imx-common/gpio.h>

#undef CONFIG_LDO_BYPASS_CHECK

#define CONFIG_MX6
#define CONFIG_ROM_UNIFIED_SECTIONS
#define CONFIG_SYS_GENERIC_BOARD

/* ATAGs */
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/* Boot options */
#if (defined(CONFIG_MX6SX) || defined(CONFIG_MX6SL) || defined(CONFIG_MX6UL))
#define CONFIG_LOADADDR		0x82000000
#ifndef CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_TEXT_BASE	0x87800000
#endif
#else
#define CONFIG_LOADADDR		0x12000000
#ifndef CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_TEXT_BASE	0x17800000
#endif
#endif

#ifndef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY	1
#endif

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX       1
#define CONFIG_BAUDRATE         115200

/* Command definition */
#include <config_cmd_default.h>

/* Filesystems and image support */
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE

/* Miscellaneous configurable options */
#undef CONFIG_CMD_IMLS
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE	512

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	32
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

#ifndef CONFIG_SYS_DCACHE_OFF
#define CONFIG_CMD_CACHE
#endif

/* GPIO */
#define CONFIG_CMD_GPIO

/* MMC */
#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC

/* SPL options */
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_MMC_SUPPORT
#define CONFIG_SPL_DMA_SUPPORT
#include "imx6_spl.h"

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(16 * SZ_1M)

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_MXC_GPIO

#define CONFIG_MXC_UART
#define CONFIG_MXC_UART_BASE		UART1_BASE

/* MMC Configs */
#ifdef CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	USDHC2_BASE_ADDR

/* NAND pin conflicts with usdhc2 */
#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_FSL_USDHC_NUM	1
#else
#define CONFIG_SYS_FSL_USDHC_NUM	2
#endif
#endif

#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(boot),16m(kernel),16m(dtb),-(rootfs) "
#else
#define CONFIG_MFG_NAND_PARTITION ""
#endif

/* I2C configs */
#define CONFIG_CMD_I2C
#ifdef CONFIG_CMD_I2C
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_SPEED		100000

/* PMIC only for 9X9 EVK */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE3000
#define CONFIG_POWER_PFUZE3000_I2C_ADDR  0x08
#endif

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1

#define NAND_BOOT_ENV_SETTINGS \
	"nandargs=setenv bootargs console=${console},${baudrate} " \
		"ubi.mtd=4 root=ubi0:rootfs rootfstype=ubifs rw ${cma_size}\0" \
	"nandboot=echo Booting from nand ...; " \
		"run nandargs; " \
		"run optargs; " \
		"nand read ${loadaddr} 0x600000 0x7e0000; " \
		"nand read ${fdt_addr} 0xde0000 0x20000; " \
		"bootz ${loadaddr} - ${fdt_addr}\0" \
	"mtdids=" MTDIDS_DEFAULT "\0" \
	"mtdparts=" MTDPARTS_DEFAULT "\0"


#define MMC_BOOT_ENV_SETTINGS \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=0\0" \
	"mmcautodetect=yes\0" \
	"mmcbootpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"mmcrootpart=2\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/mmcblk${mmcblk}p${mmcrootpart} rootwait rw " \
		"${cma_size}\0" \
	"loadbootenv=" \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${bootenv}\0" \
	"importbootenv=echo Importing bootenv from mmc ...; " \
		"env import -t ${loadaddr} ${filesize}\0" \
	"loadbootscript=" \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimagesize=6300000\0" \
	"loadimage=mw.b ${loadaddr} 0 ${loadimagesize}; " \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${image}\0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcbootpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run optargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi\0" \


#ifdef CONFIG_SYS_BOOT_NAND
#define BOOT_ENV_SETTINGS	NAND_BOOT_ENV_SETTINGS
#define CONFIG_BOOTCOMMAND \
	"run ramsize_check; " \
	"run nandboot || " \
	"run netboot"

#else
#define BOOT_ENV_SETTINGS	MMC_BOOT_ENV_SETTINGS
#define CONFIG_BOOTCOMMAND \
	"run ramsize_check; " \
	"mmc dev ${mmcdev};" \
	"mmc dev ${mmcdev}; if mmc rescan; then " \
		"if run loadbootenv; then " \
			"run importbootenv; " \
		"fi; " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loadimage; then " \
				"run mmcboot; " \
			"else run netboot; " \
			"fi; " \
		"fi; " \
	"else run netboot; fi"

#endif

#define OPT_ENV_SETTINGS \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	OPT_ENV_SETTINGS \
	"bootenv=uEnv.txt\0" \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttymxc0\0" \
	"fdt_file=undefined\0" \
	"fdt_addr=0x83000000\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x83100000\0" \
	"splashenable=setenv splashfile /boot/splash.bmp; " \
		"setenv splashimage 0x83100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs ${cma_size} " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"run optargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"run findfdt; " \
			"echo fdt_file=${fdt_file}; " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0" \
	"usbnet_devaddr=f8:dc:7a:00:00:02\0" \
	"usbnet_hostaddr=f8:dc:7a:00:00:01\0" \
	"ramsize_check="\
		"if test $sdram_size -lt 256; then " \
			"setenv cma_size cma=32MB; " \
			"setenv loadimagesize 1A00000; " \
			"setenv fdt_addr 0x84000000; " \
			"setenv loadaddr 0x84600000; " \
		"else " \
			"if test $sdram_size -lt 512; then " \
				"setenv cma_size cma=64MB; " \
			"fi; " \
		"fi;\0" \
	"findfdt="\
		"if test $fdt_file = undefined; then " \
			"if test $boot_dev = sd; then " \
				"if test $som_storage = emmc || test $som_storage = none; then " \
					"if test $soc_type = MX6ULL; then " \
						"setenv fdt_file imx6ull-var-dart-sd_emmc.dtb; " \
					"else " \
						"setenv fdt_file imx6ul-var-dart-sd_emmc.dtb; " \
					"fi; " \
				"fi; " \
				"if test $som_storage = nand; then " \
					"if test $soc_type = MX6ULL; then " \
						"setenv fdt_file imx6ull-var-dart-sd_nand.dtb; " \
					"else " \
						"setenv fdt_file imx6ul-var-dart-sd_nand.dtb; " \
					"fi; " \
				"fi; " \
			"fi; " \
			"if test $boot_dev = emmc; then " \
				"if test $wifi = yes; then " \
					"if test $soc_type = MX6ULL; then " \
						"if test $som_rev = 5G; then " \
							"setenv fdt_file imx6ull-var-dart-5g-emmc_wifi.dtb; " \
						"else " \
							"setenv fdt_file imx6ull-var-dart-emmc_wifi.dtb; " \
						"fi; " \
					"else " \
						"if test $som_rev = 5G; then " \
							"setenv fdt_file imx6ul-var-dart-5g-emmc_wifi.dtb; " \
						"else " \
							"setenv fdt_file imx6ul-var-dart-emmc_wifi.dtb; " \
						"fi; " \
					"fi; " \
				"else " \
					"if test $soc_type = MX6ULL; then " \
						"setenv fdt_file imx6ull-var-dart-sd_emmc.dtb; " \
					"else " \
						"setenv fdt_file imx6ul-var-dart-sd_emmc.dtb; " \
					"fi; " \
				"fi; " \
			"fi; " \
			"if test $boot_dev = nand; then " \
				"if test $wifi = yes; then " \
					"if test $soc_type = MX6ULL; then " \
						"if test $som_rev = 5G; then " \
							"setenv fdt_file imx6ull-var-dart-5g-nand_wifi.dtb; " \
						"else " \
							"setenv fdt_file imx6ull-var-dart-nand_wifi.dtb; " \
						"fi; " \
					"else " \
						"if test $som_rev = 5G; then " \
							"setenv fdt_file imx6ul-var-dart-5g-nand_wifi.dtb; " \
						"else " \
							"setenv fdt_file imx6ul-var-dart-nand_wifi.dtb; " \
						"fi; " \
					"fi; " \
				"else " \
					"if test $soc_type = MX6ULL; then " \
						"setenv fdt_file imx6ull-var-dart-sd_nand.dtb; " \
					"else " \
						"setenv fdt_file imx6ul-var-dart-sd_nand.dtb; " \
					"fi; " \
				"fi; " \
			"fi; " \
			"if test $fdt_file = undefined; then " \
				"echo WARNING: Could not determine dtb to use; " \
			"fi; " \
		"fi;\0"


/* Miscellaneous configurable options */
#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x10000000)

#define CONFIG_CMD_SETEXPR
#define CONFIG_CMD_TIME

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR
#define CONFIG_SYS_HZ			1000

#define CONFIG_CMDLINE_EDITING
#define CONFIG_STACKSIZE		SZ_128K

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

#define CONFIG_ENV_SIZE			SZ_8K

#ifdef CONFIG_SYS_BOOT_QSPI
#define CONFIG_SYS_USE_QSPI
#define CONFIG_ENV_IS_IN_SPI_FLASH
#elif defined CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
#else
#define CONFIG_SYS_USE_QSPI
#define CONFIG_ENV_IS_IN_MMC
#endif


#define CONFIG_SYS_MMC_ENV_DEV		0   	/* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */

#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_ENV_OFFSET		(8 * SZ_64K)
#elif defined(CONFIG_ENV_IS_IN_NAND)
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET		0x00400000
#define CONFIG_ENV_SECT_SIZE		0x00200000
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#endif


#define CONFIG_OF_LIBFDT
#define CONFIG_CMD_BOOTZ
#define CONFIG_CMD_BMODE

#ifndef CONFIG_SYS_DCACHE_OFF
#define CONFIG_CMD_CACHE
#endif

/* #define CONFIG_FSL_QSPI */
#ifdef CONFIG_FSL_QSPI
#define CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_STMICRO
#define CONFIG_SPI_FLASH_BAR
#define CONFIG_SF_DEFAULT_BUS		0
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_SPEED	40000000
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define FSL_QSPI_FLASH_NUM		1
#define FSL_QSPI_FLASH_SIZE		SZ_32M
#endif

#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_CMD_NAND
#define CONFIG_CMD_NAND_TRIMFFS

/* UBI/UBIFS support */
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_UBI_SILENCE_MSG
#define CONFIG_UBIFS_SILENCE_MSG
#define CONFIG_RBTREE
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_LZO

#define MTDIDS_DEFAULT          "nand0=nandflash-0"

/*
 * Partitions layout for NAND is:
 *     mtd0: 2M       (spl) First boot loader
 *     mtd1: 2M       (u-boot)
 *     mtd2: 2M       (u-boot env.)
 *     mtd3: 8M       (kernel, dtb)
 *     mtd4: left     (rootfs)
 */
/* Default mtd partition table */
#define MTDPARTS_DEFAULT        "mtdparts=nandflash-0:"\
                                        "2m(spl),"\
                                        "2m(u-boot),"\
                                        "2m(u-boot-env),"\
                                        "8m(kernel),"\
                                        "-(rootfs)"     /* ubifs */

/* NAND stuff */
#define CONFIG_NAND_MXS
#define CONFIG_SPL_NAND_SUPPORT
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		0x40000000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SYS_NAND_U_BOOT_OFFS		0x200000

/* DMA stuff, needed for GPMI/MXS NAND support */
#define CONFIG_APBH_DMA
#define CONFIG_APBH_DMA_BURST
#define CONFIG_APBH_DMA_BURST8
#endif

#define CONFIG_NETCONSOLE

/* Framebuffer */
#define CONFIG_VIDEO
#ifdef CONFIG_VIDEO
#define CONFIG_CFB_CONSOLE
#define CONFIG_VIDEO_MXS
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_SW_CURSOR
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_BMP_LOGO
#endif

/* SPLASH SCREEN Configs  */
#ifdef CONFIG_SPLASH_SCREEN
/* Framebuffer and LCD  */
#define CONFIG_CFB_CONSOLE
#define CONFIG_CMD_BMP
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_SPLASH_SOURCE
#endif

#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_MODE \
	        "panel=VAR-WVGA-LCD\0"
#else
#define CONFIG_VIDEO_MODE ""
#endif

/* USB Configs */
#define CONFIG_CMD_USB
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_USB_ETHER_MCS7830
#define CONFIG_USB_ETHER_SMSC95XX
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS   0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2

#define CONFIG_CI_UDC
#define CONFIG_USBD_HS
#define CONFIG_USB_GADGET_DUALSPEED

#define CONFIG_USB_ETHER
#define CONFIG_USB_ETH_CDC

#define CONFIG_USB_GADGET
#define CONFIG_CMD_USB_MASS_STORAGE
#define CONFIG_USB_GADGET_MASS_STORAGE
#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_USB_GADGET_VBUS_DRAW     2

#define CONFIG_G_DNL_VENDOR_NUM         0x0525
#define CONFIG_G_DNL_PRODUCT_NUM        0xa4a5
#define CONFIG_G_DNL_MANUFACTURER       "Variscite"
#endif /* CONFIG_CMD_USB */

#ifndef CONFIG_CMD_NET
#define CONFIG_CMD_NET		1
#endif
#ifdef CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_FUSE
#define CONFIG_MXC_OCOTP
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_ENET_DEV		0

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR          0x1
#define CONFIG_FEC_XCV_TYPE             RMII
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		0x3
#define CONFIG_FEC_XCV_TYPE		RMII
#endif
#define CONFIG_ETHPRIME			"FEC"

#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL
#endif

#define CONFIG_IMX_THERMAL

#endif
