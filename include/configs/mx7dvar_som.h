/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright (C) 2016 Variscite Ltd.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * Configuration settings for the Variscite i.MX7D VAR-SOM-MX7 board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_VAR_SOM_CONFIG_H
#define __MX7D_VAR_SOM_CONFIG_H

#include <asm/arch/imx-regs.h>
#include <linux/sizes.h>
#include "mx7_common.h"
#include <asm/imx-common/gpio.h>

#define CONFIG_MX7
#define CONFIG_ROM_UNIFIED_SECTIONS
#define CONFIG_SYS_GENERIC_BOARD
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_DBG_MONITOR

#ifdef CONFIG_SECURE_BOOT
#ifndef CONFIG_CSF_SIZE
#define CONFIG_CSF_SIZE			0x2000
#endif
#endif

#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(32 * SZ_1M)

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_MXC_GPIO
#define CONFIG_CMD_GPIO

#define CONFIG_SILENT_CONSOLE

#define CONFIG_MXC_UART
#define CONFIG_MXC_UART_BASE		UART1_IPS_BASE_ADDR

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX		1
#define CONFIG_BAUDRATE			115200

#define CONFIG_CMD_FUSE
#ifdef CONFIG_CMD_FUSE
#define CONFIG_MXC_OCOTP
#endif

/* MMC Configs */
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	0

#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_DOS_PARTITION
/*#define CONFIG_SUPPORT_EMMC_BOOT*/ /* eMMC specific */

#define PHYS_SDRAM_SIZE			SZ_1G

/* Ethernet Configs */
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define CONFIG_FEC_XCV_TYPE		RGMII
#define CONFIG_ETHPRIME			"FEC0"
#define CONFIG_PHYLIB
#define CONFIG_PHY_ATHEROS
#define CONFIG_FEC_DMA_MINALIGN		64

#define CONFIG_FEC_ENET_DEV 0

#if (CONFIG_FEC_ENET_DEV == 0)
#define IMX_FEC_BASE			ENET_IPS_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		0x0
#elif (CONFIG_FEC_ENET_DEV == 1)
#define IMX_FEC_BASE			ENET2_IPS_BASE_ADDR
#define CONFIG_FEC_MXC_PHYADDR		0x1
#endif

#define CONFIG_FEC_MXC_MDIO_BASE	ENET_IPS_BASE_ADDR

/* Framebuffer */
#define CONFIG_VIDEO
#ifdef CONFIG_VIDEO
#define	CONFIG_CFB_CONSOLE
#define	CONFIG_VIDEO_MXS
#define	CONFIG_VIDEO_LOGO
#define	CONFIG_VIDEO_SW_CURSOR
#define	CONFIG_VGA_AS_SINGLE_DEVICE
#define	CONFIG_SYS_CONSOLE_IS_IN_ENV
#define	CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define	CONFIG_CMD_BMP
#define	CONFIG_BMP_16BPP
#define	CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_BMP_LOGO
#endif

#define CONFIG_SPLASH_SCREEN
/* SPLASH SCREEN Configs  */
#ifdef CONFIG_SPLASH_SCREEN
/* Framebuffer and LCD  */
#define	CONFIG_CFB_CONSOLE
#define CONFIG_CMD_BMP
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_SPLASH_SOURCE
#endif

/* PMIC */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE300
#define CONFIG_POWER_PFUZE300_I2C_ADDR	0x08

#undef CONFIG_BOOTM_NETBSD
#undef CONFIG_BOOTM_PLAN9
#undef CONFIG_BOOTM_RTEMS

#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_IMPORTENV

/* I2C configs */
#define CONFIG_CMD_I2C
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_SPEED		100000

/* Command definition */
#include <config_cmd_default.h>

#undef CONFIG_CMD_IMLS

#define CONFIG_BOOTDELAY		1

#define CONFIG_LOADADDR			0x80800000
#define CONFIG_SYS_TEXT_BASE		0x87800000

#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_SYS_USE_NAND
#define CONFIG_ENV_IS_IN_NAND
#else
#define CONFIG_ENV_IS_IN_MMC
#endif

#define CONFIG_SYS_AUXCORE_BOOTDATA 0x7F8000 /* Set to TCML address */
#define CONFIG_CMD_BOOTAUX /* Boot M4 */
#define CONFIG_CMD_SETEXPR

#ifdef CONFIG_CMD_BOOTAUX
#define CONFIG_MXC_RDC /* Enable RDC to isolate the peripherals for A7 and M4 */

#define UPDATE_M4_ENV \
	"m4bootdata="__stringify(CONFIG_SYS_AUXCORE_BOOTDATA)"\0" \
	"m4image=m4_qspi.bin\0" \
	"loadm4image=fatload mmc ${mmcdev}:${mmcpart} ${m4bootdata} ${m4image}\0" \
	"m4boot=run loadm4image; dcache flush; bootaux ${m4bootdata}\0"
#else
#define UPDATE_M4_ENV ""
#endif

#define CONFIG_SYS_MMC_IMG_LOAD_PART	1
#ifdef CONFIG_SYS_BOOT_NAND
#define CONFIG_MFG_NAND_PARTITION "mtdparts=gpmi-nand:64m(boot),16m(kernel),16m(dtb),-(rootfs) "
#else
#define CONFIG_MFG_NAND_PARTITION ""
#endif

#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_MODE \
	"panel=VAR-WVGA-LCD\0"
#else
#define CONFIG_VIDEO_MODE ""
#endif

#define CONFIG_MFG_ENV_SETTINGS \
	"mfgtool_args=setenv bootargs console=${console},${baudrate} " \
		"rdinit=/linuxrc " \
		"g_mass_storage.stall=0 g_mass_storage.removable=1 " \
		"g_mass_storage.file=/fat g_mass_storage.ro=1 " \
		"g_mass_storage.idVendor=0x066F g_mass_storage.idProduct=0x37FF "\
		"g_mass_storage.iSerialNumber=\"\" "\
		CONFIG_MFG_NAND_PARTITION \
		"clk_ignore_unused "\
		"\0" \
	"initrd_addr=0x83800000\0" \
	"initrd_high=0xffffffff\0" \
	"bootcmd_mfg=run mfgtool_args;bootz ${loadaddr} ${initrd_addr} ${fdt_addr};\0" \


#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#define MMC_BOOT_ENV_SETTINGS \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"mmcroot=" CONFIG_MMCROOT " rootwait rw\0" \
	"mmcautodetect=yes\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=${mmcroot}\0" \
	"loadbootscript=" \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=run findfdt; " \
		"fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
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
		"fi;\0"


#define NAND_BOOT_ENV_SETTINGS \
	"nandargs=setenv bootargs console=${console},${baudrate} ubi.mtd=4 " \
		"root=ubi0:rootfs rootfstype=ubifs rw\0" \
	"bootcmd=run nandargs; "\
		"nand read ${loadaddr} 0x600000 0x7e0000;" \
		"nand read ${fdt_addr} 0xde0000 0x20000;" \
		"bootz ${loadaddr} - ${fdt_addr}\0" \
	"mtdids=" MTDIDS_DEFAULT "\0" \
	"mtdparts=" MTDPARTS_DEFAULT "\0"


#ifdef CONFIG_SYS_BOOT_NAND
#define BOOT_ENV_SETTINGS       NAND_BOOT_ENV_SETTINGS
#else
#define BOOT_ENV_SETTINGS       MMC_BOOT_ENV_SETTINGS
#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev};" \
	"mmc dev ${mmcdev}; if mmc rescan; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loadimage; then " \
				"run mmcboot; " \
			"else " \
				"run netboot; " \
			"fi; " \
		"fi; " \
	"else run netboot; fi"
#endif


#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS \
	UPDATE_M4_ENV \
	BOOT_ENV_SETTINGS \
	CONFIG_VIDEO_MODE \
	"console=ttymxc0\0" \
	"boot_fdt=try\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=undefined\0" \
	"fdt_addr=0x83000000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=" __stringify(CONFIG_SYS_MMC_IMG_LOAD_PART) "\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x83100000\0" \
	"splashenable=setenv splashfile /boot/splash.bmp; " \
		"setenv splashimage 0x83100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0" \
	"ip_dyn=yes\0" \
	"use_m4=no\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"run findfdt; " \
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
	"findfdt="\
		"if test $fdt_file = undefined; then " \
			"if test $som_rev = EMMC; then " \
				"if test ${use_m4} = yes; then " \
					"setenv fdt_file imx7d-var-som-emmc-m4.dtb; " \
				"else " \
					"setenv fdt_file imx7d-var-som-emmc.dtb; " \
				"fi; " \
			"fi; " \
			"if test $som_rev = NAND; then " \
				"if test ${use_m4} = yes; then " \
					"setenv fdt_file imx7d-var-som-nand-m4.dtb; " \
				"else " \
					"setenv fdt_file imx7d-var-som-nand.dtb; " \
				"fi; " \
			"fi; " \
			"if test $fdt_file = undefined; then " \
				"echo WARNING: Could not determine dtb to use; " \
			"fi; " \
		"fi;\0"

/* Miscellaneous configurable options */
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT		"=> "
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE		1024

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		256
#define CONFIG_SYS_BARGSIZE CONFIG_SYS_CBSIZE

#define CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x20000000)

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

#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_CMD_NAND
#define CONFIG_CMD_NAND_TRIMFFS

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

#define MTDIDS_DEFAULT		"nand0=nandflash-0"

/*
 * Partitions layout for NAND is:
 *     mtd0: 2M       (spl) First boot loader
 *     mtd1: 2M       (u-boot)
 *     mtd2: 2M       (u-boot environment)
 *     mtd3: 8M       (kernel)
 *     mtd4: left     (rootfs)
 */
/* Default mtd partition table */
#define MTDPARTS_DEFAULT	"mtdparts=nandflash-0:"\
					"2m(spl),"\
					"2m(u-boot),"\
					"2m(u-boot_env),"\
					"8m(kernel),"\
					"-(rootfs)"     /* ubifs */

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
#endif

#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(8 * SZ_64K)
#elif defined(CONFIG_ENV_IS_IN_NAND)
#undef CONFIG_ENV_SIZE
#define CONFIG_ENV_OFFSET		(4 << 20)
#define CONFIG_ENV_SECT_SIZE		(128 << 10)
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#endif

#ifdef CONFIG_SYS_USE_NAND
#define CONFIG_SYS_FSL_USDHC_NUM	1
#else
#define CONFIG_SYS_FSL_USDHC_NUM	2
#endif
#define CONFIG_SYS_MMC_ENV_DEV		0	/* USDHC1 */
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */
#define CONFIG_MMCROOT			"/dev/mmcblk0p2"  /* USDHC1 */

#define CONFIG_OF_LIBFDT
#define CONFIG_CMD_BOOTZ

#define CONFIG_CMD_BMODE

#ifndef CONFIG_SYS_DCACHE_OFF
#define CONFIG_CMD_CACHE
#endif

#define CONFIG_CMD_TIME

#define CONFIG_IMX_THERMAL

/* USB Configs */
#define CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX7
#define CONFIG_USB_STORAGE
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS   0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2

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
#include "mx7dvar_som_android.h"
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

#endif	/* __MX7D_VAR_SOM_CONFIG_H */
