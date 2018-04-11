/*
 * Copyright (C) 2012-2015 Freescale Semiconductor, Inc.
 * Copyright (C) 2016-2017 Variscite Ltd.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * Configuration settings for Variscite VAR-SOM-MX6 board family.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __MX6VAR_SOM_CONFIG_H
#define __MX6VAR_SOM_CONFIG_H

#ifdef CONFIG_SPL
#include "imx6_spl.h"
#endif

/* Reserve 4Bytes in OCRAM for sending RAM size from SPL to U-Boot */
#undef CONFIG_SPL_MAX_SIZE
#define CONFIG_SPL_MAX_SIZE	0xFFFC  /* ==0x10000-0x4 */
#define RAM_SIZE_ADDR	((CONFIG_SPL_TEXT_BASE) + (CONFIG_SPL_MAX_SIZE))

#include "mx6_common.h"

#define CONFIG_IMX_THERMAL

#define CONFIG_MXC_UART_BASE		UART1_BASE
#define CONSOLE_DEV			"ttymxc0"

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(16 * SZ_1M)

#define CONFIG_MXC_UART

#define LOW_POWER_MODE_ENABLE

/* Falcon Mode */
#define CONFIG_CMD_SPL
#ifdef CONFIG_SPL_OS_BOOT
#define CONFIG_SPL_FS_LOAD_ARGS_NAME	"args"
#define CONFIG_SPL_FS_LOAD_KERNEL_NAME	"uImage"
#define CONFIG_SYS_SPL_ARGS_ADDR	0x18000000
#define CONFIG_CMD_SPL_WRITE_SIZE	(128 * SZ_1K)

/* Falcon Mode - MMC support: args@11MB kernel@4MB */
#define CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTOR	0x5800	/* 11MB */
#define CONFIG_SYS_MMCSD_RAW_MODE_ARGS_SECTORS	(CONFIG_CMD_SPL_WRITE_SIZE / 512)
#define CONFIG_SYS_MMCSD_RAW_MODE_KERNEL_SECTOR	0x2000	/* 4MB */

#ifdef CONFIG_NAND_BOOT
/* Falcon Mode - NAND support: args@11MB kernel@4MB */
#define CONFIG_SYS_NAND_SPL_KERNEL_OFFS	(4 * SZ_1M)
#define CONFIG_CMD_SPL_NAND_OFS		(11 * SZ_1M)
#endif
#endif

/* MMC Configs */
#define CONFIG_SYS_FSL_ESDHC_ADDR	0
#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_MMC_ENV_DEV		0
#ifndef CONFIG_SYS_MMC_ENV_PART
#define CONFIG_SYS_MMC_ENV_PART		0
#endif

#define CONFIG_FAT_WRITE

/* Ethernet Configs */
#define CONFIG_FEC_MXC
#define CONFIG_MII
#define IMX_FEC_BASE			ENET_BASE_ADDR
#define CONFIG_FEC_XCV_TYPE		RGMII
#ifdef CONFIG_DM_ETH
#define CONFIG_ETHPRIME			"eth0"
#else
#define CONFIG_ETHPRIME			"FEC"
#endif
#define CONFIG_FEC_MXC_PHYADDR		7
#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#ifdef CONFIG_NAND_BOOT
#define MMC_ROOT_PART	1
#else
#define MMC_ROOT_PART	2
#endif

#define MMC_BOOT_ENV_SETTINGS \
	"bootenv=uEnv.txt\0" \
	"script=boot.scr\0" \
	"uimage=uImage\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV) "\0" \
	"mmcblk=0\0" \
	"mmcautodetect=yes\0" \
	"mmcbootpart=1\0" \
	"mmcrootpart=" __stringify(MMC_ROOT_PART) "\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/mmcblk${mmcblk}p${mmcrootpart} rootwait rw\0" \
	"loadbootenv=" \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${bootenv};\0" \
	"importbootenv=echo Importing bootenv from mmc ...; " \
		"env import -t ${loadaddr} ${filesize}\0" \
	"loadbootscript=" \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loaduimage=load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${uimage}\0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcbootpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run videoargs; " \
		"run optargs; " \
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
		"fi;\0"


#define MMC_BOOTCMD \
	"mmc dev ${mmcdev};" \
	"if mmc rescan; then " \
		"if run loadbootenv; then " \
			"run importbootenv; " \
		"fi; " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loaduimage; then " \
				"run mmcboot; " \
			"else " \
				"run netboot; " \
			"fi; " \
		"fi; " \
	"else run netboot; fi;"

#ifdef CONFIG_NAND_BOOT
#define NAND_BOOT_ENV_SETTINGS \
	"nandargs=setenv bootargs console=${console},${baudrate} ubi.mtd=3 " \
		"root=ubi0:rootfs rootfstype=ubifs\0" \
	"rootfs_device=nand\0" \
	"boot_device=nand\0" \
	"nandboot=nand read ${loadaddr} 0x400000 0x800000; " \
		"nand read ${fdt_addr} 0x3e0000 0x20000; " \
		"bootm ${loadaddr} - ${fdt_addr};\0" \
	"bootcmd=" \
		"if test ${rootfs_device} != emmc; then " \
			"run nandargs; " \
			"run videoargs; " \
			"run optargs; " \
			"echo booting from nand ...; " \
			"run nandboot; " \
		"else " \
			"if test ${boot_device} != emmc; then " \
				"run mmcargs; " \
				"run videoargs; " \
				"run optargs; " \
				"echo booting from nand (rootfs on emmc)...; " \
				"run nandboot; " \
			"else " \
				"setenv mmcdev 1; " \
				MMC_BOOTCMD \
			"fi; " \
		"fi;\0" \
	"mtdids=" MTDIDS_DEFAULT "\0" \
	"mtdparts=" MTDPARTS_DEFAULT "\0"
#else
#define NAND_BOOT_ENV_SETTINGS ""
#define CONFIG_BOOTCOMMAND	MMC_BOOTCMD
#endif

#define OPT_ENV_SETTINGS \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0"

#define VIDEO_ENV_SETTINGS \
	"videoargs=" \
		"if hdmidet; then " \
			"setenv bootargs ${bootargs} " \
				"video=mxcfb0:dev=hdmi,1920x1080M@60,if=RGB24; " \
		"else " \
			"setenv bootargs ${bootargs} " \
				"video=mxcfb0:dev=ldb; " \
		"fi; " \
		"setenv bootargs ${bootargs} " \
			"video=mxcfb1:off video=mxcfb2:off video=mxcfb3:off;\0"


#define CONFIG_EXTRA_ENV_SETTINGS \
	MMC_BOOT_ENV_SETTINGS \
	NAND_BOOT_ENV_SETTINGS \
	VIDEO_ENV_SETTINGS \
	OPT_ENV_SETTINGS \
	"fdt_file=undefined\0" \
	"fdt_addr=0x18000000\0" \
	"fdt_high=0xffffffff\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x18100000\0" \
	"splashenable=setenv splashfile /boot/splash.bmp; " \
		"setenv splashimage 0x18100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0" \
	"console=" CONSOLE_DEV "\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs rw " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp; " \
		"run videoargs\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"run optargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${uimage}; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"run findfdt; " \
			"echo fdt_file=${fdt_file}; " \
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
	"findfdt="\
		"if test $fdt_file = undefined; then " \
			"if test $board_name = DT6CUSTOM && test $board_rev = MX6Q; then " \
				"setenv fdt_file imx6q-var-dart.dtb; " \
			"fi; " \
			"if test $board_name = SOLOCUSTOM && test $board_rev = MX6QP; then " \
				"setenv fdt_file imx6qp-var-som-vsc.dtb; " \
			"fi; " \
			"if test $board_name = SOLOCUSTOM && test $board_rev = MX6Q; then " \
				"setenv fdt_file imx6q-var-som-vsc.dtb; " \
			"fi; " \
			"if test $board_name = SOLOCUSTOM && test $board_rev = MX6DL && test $board_som = SOM-SOLO; then " \
				"setenv fdt_file imx6dl-var-som-solo-vsc.dtb; " \
			"fi; " \
			"if test $board_name = SOLOCUSTOM && test $board_rev = MX6DL && test $board_som = SOM-MX6; then " \
				"setenv fdt_file imx6dl-var-som-vsc.dtb; " \
			"fi; " \
			"if test $board_name = MX6CUSTOM && test $board_rev = MX6QP; then " \
				"i2c dev 2; " \
				"if i2c probe 0x38; then " \
					"setenv fdt_file imx6qp-var-som-cap.dtb; " \
				"else " \
					"setenv fdt_file imx6qp-var-som-res.dtb; " \
				"fi; " \
			"fi; " \
			"if test $board_name = MX6CUSTOM && test $board_rev = MX6Q; then " \
				"i2c dev 2; " \
				"if i2c probe 0x38; then " \
					"setenv fdt_file imx6q-var-som-cap.dtb; " \
				"else " \
					"setenv fdt_file imx6q-var-som-res.dtb; " \
				"fi; " \
			"fi; " \
			"if test $board_name = MX6CUSTOM && test $board_rev = MX6DL && test $board_som = SOM-SOLO; then " \
				"i2c dev 2; " \
				"if i2c probe 0x38; then " \
					"setenv fdt_file imx6dl-var-som-solo-cap.dtb; " \
				"else " \
					"setenv fdt_file imx6dl-var-som-solo-res.dtb; " \
				"fi; " \
			"fi; " \
			"if test $board_name = MX6CUSTOM && test $board_rev = MX6DL && test $board_som = SOM-MX6; then " \
				"i2c dev 2; " \
				"if i2c probe 0x38; then " \
					"setenv fdt_file imx6dl-var-som-cap.dtb; " \
				"else " \
					"setenv fdt_file imx6dl-var-som-res.dtb; " \
				"fi; " \
			"fi; " \
			"if test $fdt_file = undefined; then " \
				"echo WARNING: Could not determine dtb to use; " \
			"fi; " \
		"fi;\0"


#define CONFIG_ARP_TIMEOUT		200UL

#define CONFIG_SYS_MEMTEST_START	0x10000000
#define CONFIG_SYS_MEMTEST_END		0x10010000
#define CONFIG_SYS_MEMTEST_SCRATCH	0x10800000

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

#ifdef CONFIG_NAND_BOOT
#define CONFIG_CMD_NAND
#define CONFIG_ENV_IS_IN_NAND
#define CONFIG_SYS_NAND_U_BOOT_OFFS	0x200000
#else
#define CONFIG_ENV_IS_IN_MMC
#endif

/* Environment organization */
#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(0x3E0000)
#define CONFIG_ENV_SIZE			SZ_8K
#elif defined(CONFIG_ENV_IS_IN_NAND)
#define CONFIG_ENV_OFFSET		(0x180000)
#define CONFIG_ENV_SIZE			SZ_128K
#endif

#ifdef CONFIG_CMD_NAND
/* NAND flash command */
#define CONFIG_CMD_NAND_TRIMFFS

/* UBI/UBIFS support */
#define CONFIG_CMD_UBIFS
#define CONFIG_UBI_SILENCE_MSG
#define CONFIG_RBTREE
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_LZO

#define MTDIDS_DEFAULT		"nand0=nandflash-0"

/*
 * Partitions layout for NAND is:
 *     mtd0: 2M       (spl) First boot loader
 *     mtd1: 2M       (u-boot, dtb)
 *     mtd2: 8M       (kernel)
 *     mtd3: left     (rootfs)
 */
/* Default mtd partition table */
#define MTDPARTS_DEFAULT	"mtdparts=nandflash-0:"\
					"2m(spl),"\
					"2m(u-boot),"\
					"8m(kernel),"\
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

/* Framebuffer */
#ifndef CONFIG_SPL_BUILD
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_IPUV3
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_SPLASH_SOURCE
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
#elif defined(CONFIG_VIDEO_HDMI)
#define CONFIG_IMX_HDMI
#define CONFIG_CMD_HDMIDETECT
#endif
#endif

#define PMIC_I2C_BUS		1
#define MX6CB_CDISPLAY_I2C_BUS	2
#define MX6CB_CDISPLAY_I2C_ADDR	0x38

/* I2C Configs */
#ifndef CONFIG_DM_I2C
#define CONFIG_SYS_I2C
#endif
#ifdef CONFIG_CMD_I2C
#define CONFIG_SYS_I2C_MXC
#define CONFIG_SYS_I2C_MXC_I2C1		/* enable I2C bus 1 */
#define CONFIG_SYS_I2C_MXC_I2C2		/* enable I2C bus 2 */
#define CONFIG_SYS_I2C_MXC_I2C3		/* enable I2C bus 3 */
#define CONFIG_SYS_I2C_SPEED		100000
#endif

/* PMIC */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE100
#define CONFIG_POWER_PFUZE100_I2C_ADDR	0x08

#define CONFIG_NETCONSOLE

/* USB Configs */
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_MX6
#define CONFIG_EHCI_HCD_INIT_AFTER_RESET
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_MXC_USB_FLAGS		0
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2 /* Enabled USB controller number */

#define CONFIG_USBD_HS

/* Uncomment for USB Ethernet Gadget support */
/*
 * #define CONFIG_USB_ETHER
 * #define CONFIG_USB_ETH_CDC
 */

#define CONFIG_USB_FUNCTION_MASS_STORAGE
#endif /* CONFIG_CMD_USB */

#if defined(CONFIG_ANDROID_SUPPORT)
#include "mx6var_som_android.h"
#endif

#endif	/* __MX6VAR_SOM_CONFIG_H */
