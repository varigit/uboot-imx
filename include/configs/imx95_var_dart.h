/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 NXP
 * Copyright 2024-2025 Variscite Ltd.
 */

#ifndef __IMX95_VAR_DART_H
#define __IMX95_VAR_DART_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "env/nxp/imx_env.h"

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV "sec_boot=yes\0"
#else
#define AHAB_ENV "sec_boot=no\0"
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(USB, usb, 0)

#include <config_distro_bootcmd.h>
#else
#define BOOTENV
#endif

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x93800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS \
	BOOTENV \
	AHAB_ENV \
	"prepare_mcore=setenv mcore_args clk_ignore_unused pd_ignore_unused;\0" \
	"cpuidle= \0" \
	"scriptaddr=0x93500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"image=Image.gz\0" \
	"img_addr=0xB0000000\0" \
	"splashimage=0xA0000000\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashsourceauto=yes\0" \
	"splashpos=m,m\0" \
	"backlight_disable=gpio clear GPIO2_25\0" \
	"backlight_enable=gpio set GPIO2_25\0" \
	"console=ttyLP0,115200 earlycon\0" \
	"fdt_addr_r=0x93000000\0" \
	"fdt_addr=0x93000000\0" \
	"fdt_high=0xffffffffffffffff\0"	 \
	"nfsroot=/srv/nfs/" CONFIG_SYS_BOARD "/rootfs\0" \
	"cntr_addr=0xA8000000\0" \
	"cntr_file=os_cntr_signed.bin\0" \
	"bootdir=/boot\0" \
	"fdt_file=undefined\0" \
	"boot_fit=no\0" \
	"bootm_size=0x10000000\0" \
	"mmcdev=" __stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcpart=1\0" \
	"mmcautodetect=yes\0" \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs ${cpuidle} ${mcore_args} console=${console} \
		root=/dev/mmcblk${mmcblk}p${mmcpart} rootwait rw\0 " \
	"script=boot.scr\0" \
	"bootenv=uEnv.txt\0" \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadbootenv=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${bootenv}\0" \
	"importbootenv=echo Importing environment from mmc ...; " \
		"env import -t -r $loadaddr $filesize\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		"unzip ${img_addr} ${loadaddr}\0" \
	"findfdt=" \
		"if test $fdt_file = undefined; then " \
			"setenv module_name imx95-var-dart; " \
			"if test $board_name = DART-MX95_V2; then " \
				"setenv module_name imx95-var-dart-v2; " \
			"fi; " \
			"setenv wbe_suffix; " \
			"if test $carrier_name = sonata && test $som_has_wbe = 1; then " \
				"setenv wbe_suffix -wbe; " \
			"fi; " \
			"setenv fdt_file ${module_name}${wbe_suffix}-${carrier_name}${m7_dtb_suffix}.dtb;" \
		"fi; \0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcpart} ${fdt_addr_r} ${bootdir}/${fdt_file}\0" \
	"loadcntr=load mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${bootdir}/${cntr_file}\0" \
	"auth_os=booti ${cntr_addr}\0" \
	"boot_os=booti ${loadaddr} - ${fdt_addr_r};\0" \
	"m7_load_addr=0x90000000\0" \
	"m7_addr=0x203c0000\0" \
	"m7_addr_auxview=0x00000000\0" \
	"m7_bin=hello_world.bin\0" \
	"use_m7=no\0" \
	"loadm7bin=load mmc ${mmcdev}:${mmcpart} ${m7_load_addr} ${bootdir}/${m7_bin} && cp.b ${m7_load_addr} ${m7_addr} ${filesize};\0" \
	"runm7bin=echo Booting M7 from TCM; stopaux 1; prepaux core 1; bootaux ${m7_addr_auxview} 1;\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run optargs; " \
		"run backlight_disable; " \
		"if test ${sec_boot} = yes; then " \
			"run auth_os; " \
		"else " \
			"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
				"bootm ${loadaddr}; " \
			"else " \
				"if run loadfdt; then " \
					"run boot_os; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi;" \
		"fi;\0" \
		"netargs=setenv bootargs ${mcore_args} console=${console} " \
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs;  " \
		"run optargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"if test ${sec_boot} = yes; then " \
			"${get_cmd} ${cntr_addr} ${cntr_file}; " \
			"run auth_os; " \
		"else " \
			"${get_cmd} ${img_addr} ${image}; unzip ${img_addr} ${loadaddr}; " \
			"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
				"bootm ${loadaddr}; " \
			"else " \
				"run findfdt; " \
				"if ${get_cmd} ${fdt_addr_r} ${fdt_file}; then " \
					"run boot_os; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi;" \
		"fi;\0" \
	"bsp_bootcmd=echo Running BSP bootcmd ...; " \
		"if env exists mender_setup; then " \
			"run mender_setup; " \
		"fi; " \
		"mmc dev ${mmcdev}; if mmc rescan; then " \
			"if test ${use_m7} = yes && run loadm7bin; then " \
				"run prepare_mcore;" \
				"run runm7bin; " \
			"fi; " \
			"if run loadbootscript; then " \
				"run bootscript; " \
			"else " \
				"if test ${sec_boot} = yes; then " \
					"if run loadcntr; then " \
						"run mmcboot; " \
					"else run netboot; " \
					"fi; " \
				"else " \
					"if run loadbootenv; then " \
						"echo Loaded environment from ${bootenv}; " \
						"run importbootenv; " \
					"fi;" \
					"if run loadimage; then " \
						"run mmcboot; " \
					"else run netboot; " \
					"fi; " \
				"fi; " \
			"fi; " \
		"fi;"

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x90000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

#define CFG_SYS_SDRAM_BASE           0x90000000
#define PHYS_SDRAM                      0x90000000
/* Totally 8GB */
#define PHYS_SDRAM_SIZE			0x70000000UL /* 2GB  - 256MB DDR */
#define PHYS_SDRAM_2_SIZE 		0x180000000 /* 6GB */

#define DEFAULT_SDRAM_SIZE		(4UL * SZ_1G) /* 4GB Minimum DDR5, see get_dram_size */

#define CFG_SYS_FSL_USDHC_NUM	2

#define CFG_SYS_SECURE_SDRAM_BASE	0x8A000000 /* Secure DDR region for A55, SPL could use first 2MB */
#define CFG_SYS_SECURE_SDRAM_SIZE	0x06000000

/* Using ULP WDOG for reset */
#define WDOG_BASE_ADDR          WDG3_BASE_ADDR

/* USB configs */
#if defined(CONFIG_CMD_NET)
#define PHY_ANEG_TIMEOUT 20000
/* Number of Rx BD rings: 8 per ENETC instance */
#endif

#ifdef CONFIG_ANDROID_SUPPORT
#include "imx95_var_dart_android.h"
#endif

#endif
