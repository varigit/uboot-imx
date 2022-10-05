/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 * Copyright 2019-2022 Variscite Ltd.
 */

#ifndef __IMX8QXP_VAR_SOM_H
#define __IMX8QXP_VAR_SOM_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_MAX_SIZE				(192 * 1024)
#define CONFIG_SYS_MONITOR_LEN				(1024 * 1024)

/*
 * 0x08081000 - 0x08180FFF is for m4_0 xip image,
  * So 3rd container image may start from 0x8181000
 */
#define CONFIG_SYS_UBOOT_BASE 			0x08181000

#define CONFIG_SPL_STACK		0x013fff0
#define CONFIG_SPL_BSS_START_ADDR	0x00130000
#define CONFIG_SPL_BSS_MAX_SIZE		0x1000		/* 4 KB */
#define CONFIG_SYS_SPL_MALLOC_START	0x82200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x80000		/* 512 KB */
#define CONFIG_MALLOC_F_ADDR		0x00138000

#define CONFIG_SPL_RAW_IMAGE_ARM_TRUSTED_FIRMWARE

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#endif

#define CONFIG_CMD_READ

/* Flat Device Tree Definitions */

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV "sec_boot=yes\0"
#else
#define AHAB_ENV "sec_boot=no\0"
#endif

/* Boot M4 */
#define M4_BOOT_ENV \
	"m4_0_image=m4_0.bin\0" \
	"loadm4image_0=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${m4_0_image}\0" \
	"m4boot_0=run loadm4image_0; dcache flush; bootaux ${loadaddr} 0\0" \

#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x83100000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0" \
	"sd_dev=1\0"

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	CONFIG_MFG_ENV_SETTINGS \
	M4_BOOT_ENV \
	AHAB_ENV \
	"bootdir=/boot\0"	\
	"script=boot.scr\0" \
	"image=Image.gz\0" \
	"console=ttyLP3\0" \
	"img_addr=0x82000000\0"			\
	"fdt_addr=0x83000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"cntr_addr=0x98000000\0"			\
	"cntr_file=os_cntr_signed.bin\0" \
	"boot_fdt=try\0" \
	"fdt_file=imx8qxp-var-som-symphony.dtb\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=1\0" \
	"mmcpart=1\0" \
	"mmcautodetect=yes\0" \
	"m4_addr=0x88000000\0" \
	"m4_bin=hello_world.bin\0" \
	"use_m4=no\0" \
	"loadm4bin=load mmc ${mmcdev}:${mmcpart} ${m4_addr} ${bootdir}/${m4_bin}\0" \
	"runm4bin=" \
		"dcache flush;" \
		"bootaux ${m4_addr};\0" \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} earlycon " \
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootfstype=ext4 rootwait rw ${cma_size} cma_name=linux,cma\0 " \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		  "unzip ${img_addr} ${loadaddr}\0" \
	"loadfdt=load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"ramsize_check="\
		"if test $sdram_size -le 1024; then " \
			"setenv cma_size cma=480M@2400M; " \
		"else " \
			"setenv cma_size cma=960M@2400M; " \
		"fi;\0" \
	"loadcntr=load mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${bootdir}/${cntr_file}\0" \
	"auth_os=auth_cntr ${cntr_addr}\0" \
	"boot_os=booti ${loadaddr} - ${fdt_addr};\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run optargs; " \
		"if test ${sec_boot} = yes; then " \
			"if run auth_os; then " \
				"run boot_os; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if run loadfdt; then " \
					"run boot_os; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"else " \
				"echo wait for boot; " \
			"fi;" \
		"fi;\0" \
	"netargs=setenv bootargs console=${console},${baudrate} earlycon " \
		"root=/dev/nfs ${cma_size} cma_name=linux,cma " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs;  " \
		"run optargs;  " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if test ${sec_boot} = yes; then " \
			"${get_cmd} ${cntr_addr} ${cntr_file}; " \
			"if run auth_os; then " \
				"run boot_os; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"${get_cmd} ${img_addr} ${image}; unzip ${img_addr} ${loadaddr};" \
			"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
				"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
					"run boot_os; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"else " \
				"booti; " \
			"fi;" \
		"fi;\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x9e000000\0" \
	"splashenable=setenv splashfile ${bootdir}/splash.bmp; " \
		"setenv splashimage 0x83100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0"

/* Link Definitions */

#define CONFIG_SYS_INIT_SP_ADDR		0x80200000

/* USDHC1 is for eMMC, USDHC2 is for SD on carrier board */
#define CONFIG_SYS_FSL_USDHC_NUM	2

/* Size of malloc() pool */

#define CONFIG_SYS_SDRAM_BASE		0x80000000
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_2			0x880000000
#define DEFAULT_SDRAM_SIZE		(2048ULL * SZ_1M)

/*
 * Defined only to fix compilation errors
 *
 * Variscite get DDR memory size from eeprom without to use the defines:
 * PHYS_SDRAM_1_SIZE and PHYS_SDRAM_2_SIZE
 * However the compilation of arch/arm/mach-imx/imx8/cpu.c file generate 
 * the errors: ‘PHYS_SDRAM_1_SIZE’ and ‘PHYS_SDRAM_2_SIZE’ undeclared
*/
#define PHYS_SDRAM_1_SIZE		0x40000000	/* 1 GB */
#define PHYS_SDRAM_2_SIZE		0x40000000	/* 1 GB */

/* EEPROM */
#define VAR_EEPROM_DRAM_START		0x83000000

/* Serial */
#define CONFIG_BAUDRATE			115200

/* Monitor Command Prompt */
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		8000000	/* 8MHz */

#define CONFIG_SERIAL_TAG

#define CONFIG_USB_MAX_CONTROLLER_COUNT 2

#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_MXC_USB_PORTSC	(PORT_PTS_UTMI | PORT_PTS_PTW)
#endif

/* Networking */
#define CONFIG_FEC_ENET_DEV 0
#if (CONFIG_FEC_ENET_DEV == 0)
#define CONFIG_ETHPRIME			"eth0"
#elif (CONFIG_FEC_ENET_DEV == 1)
#define CONFIG_ETHPRIME			"eth1"
#endif

#define CONFIG_FEC_XCV_TYPE		RGMII
#define PHY_ANEG_TIMEOUT		20000

/* Splash screen */
#ifdef CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SOURCE
#define CONFIG_HIDE_LOGO_VERSION
#endif

#endif /* __IMX8QXP_VAR_SOM_H */
