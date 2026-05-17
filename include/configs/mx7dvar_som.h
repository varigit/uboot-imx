/*
 * Copyright (C) 2015-2016 Freescale Semiconductor, Inc.
 * Copyright (C) 2016-2025 Variscite Ltd.
 *
 * Author: Eran Matityahu <eran.m@variscite.com>
 *
 * Configuration settings for the Variscite i.MX7D VAR-SOM-MX7 board family.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_VAR_SOM_CONFIG_H
#define __MX7D_VAR_SOM_CONFIG_H

#include "mx7_common.h"

#define CFG_MXC_UART_BASE		UART1_IPS_BASE_ADDR
#define CFG_FEC_ENET_DEV		0
#define PHYS_SDRAM_SIZE			SZ_1G
#define CFG_SYS_AUXCORE_BOOTDATA 0x7F8000
#ifdef CONFIG_IMX_BOOTAUX
#ifdef CONFIG_NAND_BOOT
#define M4_ENV_SETTINGS \
	"loadm4image=nand read ${m4bootdata} 0x200000 0x8000\0"
#else
#define M4_ENV_SETTINGS \
	"m4image=m4_qspi.bin\0" \
	"loadm4image=load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${m4image}; " \
		"cp.b ${loadaddr} ${m4bootdata} ${filesize}\0"
#endif
#else
#define M4_ENV_SETTINGS ""
#endif

#define CONFIG_DFU_ENV_SETTINGS \
	"dfu_alt_info=image raw 0 0x800000;"\
		"u-boot raw 0 0x4000;"\
		"bootimg part 0 1;"\
		"rootfs part 0 2\0" \

#define MMC_BOOT_ENV_SETTINGS \
	CONFIG_DFU_ENV_SETTINGS \
	"bootenv=uEnv.txt\0" \
	"script=boot.scr\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=0\0" \
	"mmcautodetect=yes\0" \
	"mmcbootpart=1\0" \
	"mmcrootpart=2\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/mmcblk${mmcblk}p${mmcrootpart} rootwait rw ${cma_size}\0 " \
	"loadbootenv=" \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${bootenv}\0" \
	"importbootenv=echo Importing environment from mmc ...; " \
		"env import -t ${loadaddr} ${filesize}\0" \
	"loadbootscript=" \
		"load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${image}\0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcbootpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run ramsize_check; " \
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
		"fi;\0"


#define NAND_BOOT_ENV_SETTINGS \
	"nandargs=setenv bootargs console=${console},${baudrate} ubi.mtd=4 " \
		"root=ubi0:rootfs rootfstype=ubifs rw ${cma_size}\0" \

#ifdef CONFIG_NAND_BOOT
#define BOOT_ENV_SETTINGS       NAND_BOOT_ENV_SETTINGS
#else
#define BOOT_ENV_SETTINGS       MMC_BOOT_ENV_SETTINGS
#endif

#define OPT_ENV_SETTINGS \
	"optargs=setenv bootargs ${bootargs} ${kernelargs}\0"

#define CFG_EXTRA_ENV_SETTINGS \
	M4_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	OPT_ENV_SETTINGS \
	"console=ttymxc0\0" \
	"boot_fdt=try\0" \
	"image=zImage\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"fdt_file=undefined\0" \
	"fdt_addr=0x83000000\0" \
	"panel=VAR-WVGA-LCD\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x83100000\0" \
	"splashenable=setenv splashfile /boot/splash.bmp; " \
		"setenv splashimage 0x83100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0" \
	"splashpos=m,m\0" \
	"ip_dyn=yes\0" \
	"use_m4=no\0" \
	"m4bootdata="__stringify(CFG_SYS_AUXCORE_BOOTDATA)"\0" \
	"m4boot=if run loadm4image; then dcache flush; bootaux ${m4bootdata}; fi\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs rw ${cma_size} " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"nfsroot=/srv/nfs/" CONFIG_SYS_BOARD "/rootfs\0" \
	"netboot=echo Booting from net ...; " \
		"run ramsize_check; " \
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
	"ramsize_check="\
		"if test $sdram_size -lt 512; then " \
			"setenv cma_size cma=32MB; " \
		"elif test $sdram_size -lt 1024; then " \
			"setenv cma_size cma=64MB; " \
		"else " \
			"setenv cma_size cma=128MB; " \
		"fi;\0" \
	"findfdt="\
		"if test $fdt_file = undefined; then " \
			"if test $som_rev = EMMC; then " \
				"if test ${use_m4} = yes; then " \
					"if test -n $codec && test $codec = wm8731; then " \
						"setenv fdt_file imx7d-var-som-emmc-m4-${codec}.dtb; " \
					"else " \
						"setenv fdt_file imx7d-var-som-emmc-m4.dtb; " \
					"fi; " \
				"else " \
					"if test -n $codec && test $codec = wm8731; then " \
						"setenv fdt_file imx7d-var-som-emmc-${codec}.dtb; " \
					"else " \
						"setenv fdt_file imx7d-var-som-emmc.dtb; " \
					"fi; " \
				"fi; " \
			"fi; " \
			"if test $som_rev = NAND; then " \
				"if test ${use_m4} = yes; then " \
					"if test -n $codec && test $codec = wm8731; then " \
						"setenv fdt_file imx7d-var-som-nand-m4-${codec}.dtb; " \
					"else " \
						"setenv fdt_file imx7d-var-som-nand-m4.dtb; " \
					"fi; " \
				"else " \
					"if test -n $codec && test $codec = wm8731; then " \
						"setenv fdt_file imx7d-var-som-nand-${codec}.dtb; " \
					"else " \
						"setenv fdt_file imx7d-var-som-nand.dtb; " \
					"fi; " \
				"fi; " \
			"fi; " \
			"if test $fdt_file = undefined; then " \
				"echo WARNING: Could not determine dtb to use; " \
			"fi; " \
		"fi;\0"

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CFG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CFG_SYS_INIT_RAM_ADDR		IRAM_BASE_ADDR
#define CFG_SYS_INIT_RAM_SIZE		IRAM_SIZE

#define PHYS_MIN_SDRAM_SIZE		SZ_256M
#define VAR_EEPROM_DRAM_START		(PHYS_SDRAM + (PHYS_MIN_SDRAM_SIZE >> 1))

#ifdef CONFIG_NAND_MXS
/* NAND stuff */
#define CFG_SYS_NAND_BASE		0x40000000
#endif

/* MMC Config */
#define CFG_SYS_FSL_ESDHC_ADDR	0

#endif	/* __MX7D_VAR_SOM_CONFIG_H */
