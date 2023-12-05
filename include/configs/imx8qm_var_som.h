/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019-2024 Variscite Ltd.
 *
 */

#ifndef __IMX8QM_VAR_SOM_H
#define __IMX8QM_VAR_SOM_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#ifdef CONFIG_SPL_BUILD

/*
 * 0x08081000 - 0x08180FFF is for m4_0 xip image,
 * 0x08181000 - 0x008280FFF is for m4_1 xip image
 * So 3rd container image may start from 0x8281000
 */
#define CFG_SYS_UBOOT_BASE 0x08281000

#define CFG_MALLOC_F_ADDR		0x00138000
#endif

#ifdef CONFIG_TARGET_IMX8QM_MEK_A53_ONLY
#define IMX_HDMI_FIRMWARE_LOAD_ADDR (CFG_SYS_SDRAM_BASE + SZ_64M)
#define IMX_HDMITX_FIRMWARE_SIZE 0x20000
#define IMX_HDMIRX_FIRMWARE_SIZE 0x20000
#endif

#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_IMPORTENV
#undef CONFIG_CMD_IMLS

#undef CONFIG_CMD_CRC32

#define CFG_SYS_FSL_ESDHC_ADDR       0

#define CONFIG_PCIE_IMX
#define CONFIG_PCI_SCAN_SHOW

#define PHY_ANEG_TIMEOUT 20000

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV "sec_boot=yes\0"
#else
#define AHAB_ENV "sec_boot=no\0"
#endif

/* Boot M4 */
#define M4_BOOT_ENV \
	"m4_0_image=m4_0.bin\0" \
	"m4_1_image=m4_1.bin\0" \
	"loadm4image_0=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${m4_0_image}\0" \
	"loadm4image_1=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${m4_1_image}\0" \
	"m4boot_0=run loadm4image_0; dcache flush; bootaux ${loadaddr} 0\0" \
	"m4boot_1=run loadm4image_1; dcache flush; bootaux ${loadaddr} 1\0" \

#define HDP_LOAD_ENV \
	"if run loadhdp; then; hdp load ${hdp_addr}; fi;"
#define HDPRX_LOAD_ENV \
	"if test ${hdprx_enable} = yes; then if run loadhdprx; then; hdprx load ${hdprx_addr}; fi; fi; "
#define INITRD_ADDR_ENV "initrd_addr=0x83100000\0"

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	INITRD_ADDR_ENV \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0" \
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	M4_BOOT_ENV \
	AHAB_ENV \
	"bootdir=/boot\0" \
	"script=boot.scr\0" \
	"image=Image.gz\0" \
	"panel=VAR-WVGA-LCD\0" \
	"console=ttyLP0\0" \
	"img_addr=0x82000000\0" \
	"fdt_addr=0x83000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"cntr_addr=0x98000000\0"			\
	"cntr_file=os_cntr_signed.bin\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"fdt_file=undefined\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=1\0" \
	"mmcautodetect=yes\0" \
	"mmcpart=1\0" \
	"m40_addr=0x88000000\0" \
	"m40_bin=hello_world_m40.bin\0" \
	"use_m40=no\0" \
	"loadm40bin=load mmc ${mmcdev}:${mmcpart} ${m40_addr} ${bootdir}/${m40_bin}\0" \
	"runm40bin=" \
		"dcache flush; " \
		"bootaux ${m40_addr} 0;\0" \
	"m41_addr=0x88800000\0" \
	"m41_bin=hello_world_m41.bin\0" \
	"use_m41=no\0" \
	"loadm41bin=load mmc ${mmcdev}:${mmcpart} ${m41_addr} ${bootdir}/${m41_bin}\0" \
	"runm41bin=" \
		"dcache flush; " \
		"bootaux ${m41_addr} 1;\0" \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} earlycon som_wifi=${som_wifi} " \
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootfstype=ext4 rootwait rw\0 " \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		"unzip ${img_addr} ${loadaddr}\0" \
	"findfdt=" \
		"if test $fdt_file = undefined; then " \
			"if test $board_name = VAR-SOM-MX8; then " \
				"setenv fdt_file ${soc_id}-var-som-symphony.dtb; " \
			"else " \
				"setenv fdt_file ${soc_id}-var-spear-sp8customboard.dtb; " \
			"fi; " \
		"fi; \0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"hdp_addr=0x9c000000\0" \
	"hdprx_addr=0x9c800000\0" \
	"hdp_file=hdmitxfw.bin\0" \
	"hdprx_file=hdmirxfw.bin\0" \
	"hdprx_enable=no\0" \
	"loadhdp=load mmc ${mmcdev}:${mmcpart} ${hdp_addr} ${bootdir}/${hdp_file}\0" \
	"loadhdprx=load mmc ${mmcdev}:${mmcpart} ${hdprx_addr} ${bootdir}/${hdprx_file}\0" \
	"boot_os=booti ${loadaddr} - ${fdt_addr};\0" \
	"loadcntr=load mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${bootdir}/${cntr_file}\0" \
	"auth_os=auth_cntr ${cntr_addr}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		HDP_LOAD_ENV \
		HDPRX_LOAD_ENV \
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
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if ${get_cmd} ${hdp_addr} ${hdp_file}; then; hdp load ${hdp_addr}; fi;" \
		"if ${get_cmd} ${hdprx_addr} ${hdprx_file}; then; hdprx load ${hdprx_addr}; fi;" \
		"if test ${sec_boot} = yes; then " \
			"${get_cmd} ${cntr_addr} ${cntr_file}; " \
			"run netargs; " \
			"run optargs; " \
			"if run auth_os; then " \
				"run boot_os; " \
			"else " \
				"echo ERR: failed to authenticate; " \
			"fi; " \
		"else " \
			"${get_cmd} ${img_addr} ${image}; unzip ${img_addr} ${loadaddr};" \
			"run netargs; " \
			"run optargs; " \
			"run findfdt; " \
			"echo fdt_file=${fdt_file}; " \
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
	"splashimage=0x83100000\0" \
	"splashenable=setenv splashfile ${bootdir}/splash.bmp; " \
		"setenv splashimage 0x83100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0"

#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */

#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC1 */

#define CFG_SYS_SDRAM_BASE		0x80000000
#define CONFIG_NR_DRAM_BANKS		4
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_2			0x880000000

#define DEFAULT_SDRAM_SIZE             (4096ULL * SZ_1M)

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

#define VAR_EEPROM_DRAM_START          CONFIG_SYS_MEMTEST_END

/* Serial */
#define CONFIG_BAUDRATE			115200

/* Monitor Command Prompt */
#define CONFIG_SYS_PROMPT_HUSH_PS2     "> "

#define CONFIG_SERIAL_TAG

/* USB Config */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USBD_HS
#endif

#define CONFIG_USB_MAX_CONTROLLER_COUNT 2

/* USB OTG controller configs */
#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#endif

/* Video */
#ifdef CONFIG_DM_VIDEO
#define CONFIG_IMX_VIDEO_SKIP
#endif

/* Splash screen */
#ifdef CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SOURCE
#define CONFIG_HIDE_LOGO_VERSION
#endif

#endif /* __IMX8QM_VAR_SOM_H */
