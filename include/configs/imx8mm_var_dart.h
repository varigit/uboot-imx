/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 * Copyright 2018-2023 Variscite Ltd.
 */

#ifndef __IMX8MM_VAR_DART_H
#define __IMX8MM_VAR_DART_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#define CFG_SYS_UBOOT_BASE	\
	(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x930000
#endif

#define CONFIG_SERIAL_TAG

/* ENET Config */
#if defined(CONFIG_FEC_MXC)
#define PHY_ANEG_TIMEOUT 20000
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 2)

#include <config_distro_bootcmd.h>
/* redefine BOOTENV_EFI_SET_FDTFILE_FALLBACK to use Variscite function to load fdt */
#undef BOOTENV_EFI_SET_FDTFILE_FALLBACK
#define BOOTENV_EFI_SET_FDTFILE_FALLBACK \
	"setenv efi_dtb_prefixes; " \
	"run loadfdt; "
#else
#define BOOTENV
#endif

#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS \
	"bootdir=/boot\0"	\
	BOOTENV \
	"scriptaddr=0x43500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"bsp_script=boot.scr\0" \
	"image=Image.gz\0" \
	"img_addr=0x42000000\0"	\
	"console=ttymxc0,115200\0" \
	"fdt_addr_r=0x43000000\0" \
	"fdt_addr=0x43000000\0"	\
	"fdt_high=0xffffffffffffffff\0" \
	"boot_fdt=try\0" \
	"boot_fit=no\0" \
	"fdt_file=undefined\0" \
	"ip_dyn=yes\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=1\0" \
	"mmcpart=1\0" \
	"mmcautodetect=yes\0" \
	"m4_addr=0x7e0000\0" \
	"m4_bin=hello_world.bin\0" \
	"use_m4=no\0" \
	"dfu_alt_info=mmc 2=1 raw 0x42 0x1000 mmcpart\0" \
	"loadm4bin=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${m4_bin}; " \
		"cp.b ${loadaddr} ${m4_addr} ${filesize}; " \
		"echo Init rsc_table region memory; " \
		"mw.b 400ff000 0 10\0" \
	"runm4bin=" \
		"if test ${m4_addr} = 0x7e0000; then " \
			"echo Booting M4 from TCM; " \
		"else " \
			"echo Booting M4 from DRAM; " \
			"dcache flush; " \
		"fi; " \
		"bootaux ${m4_addr};\0" \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs console=${console} " \
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootwait rw ${cma_size} cma_name=linux,cma\0 " \
	"bootenv=uEnv.txt\0" \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${bsp_script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadbootenv=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${bootenv}\0" \
	"importbootenv=echo Importing environment from mmc ...; " \
		"env import -t -r $loadaddr $filesize\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		"unzip ${img_addr} ${loadaddr}\0" \
	"findfdt=" \
		"if test $fdt_file = undefined; then " \
			"if test $board_name = VAR-SOM-MX8M-MINI; then " \
				"setenv fdt_file imx8mm-var-som-symphony.dtb; " \
			"else " \
				"if test ${som_rev} -lt 2; then " \
					"setenv fdt_file imx8mm-var-dart-1.x-dt8mcustomboard.dtb; " \
				"elif test ${som_has_wbe} = 1; then " \
					"setenv fdt_file imx8mm-var-dart-wbe-dt8mcustomboard.dtb; " \
				"else " \
					"setenv fdt_file imx8mm-var-dart-dt8mcustomboard.dtb; " \
				"fi; " \
			"fi; " \
		"fi; \0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"ramsize_check="\
		"if test $sdram_size -le 512; then " \
			"setenv cma_size cma=320M; " \
		"elif test $sdram_size -le 1024; then " \
			"setenv cma_size cma=576M; " \
		"else " \
			"setenv cma_size cma=640M; " \
		"fi;\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run optargs; " \
		"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
			"bootm ${loadaddr}; " \
		"elif run loadfdt; then " \
			"booti ${loadaddr} - ${fdt_addr_r}; " \
		"else " \
			"echo WARN: Cannot load the DT; " \
		"fi;\0" \
	"netargs=setenv bootargs console=${console} " \
		"root=/dev/nfs ${cma_size} cma_name=linux,cma " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp ${cma_size}\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs;  " \
		"run optargs;  " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${img_addr} ${image}; unzip ${img_addr} ${loadaddr};" \
		"if test ${boot_fit} = yes || test ${boot_fit} = try; then " \
			"bootm ${loadaddr}; " \
		"else " \
			"run findfdt; " \
			"echo fdt_file=${fdt_file}; " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"booti ${loadaddr} - ${fdt_addr_r}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"fi;\0" \
	"bsp_bootcmd=echo Running BSP bootcmd ...; " \
	"run ramsize_check; " \
	"mmc dev ${mmcdev}; "\
	"if mmc rescan; then " \
		"if test ${use_m4} = yes && run loadm4bin; then " \
			"run runm4bin; " \
		"fi; " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loadbootenv; then " \
				"echo Loaded environment from ${bootenv}; " \
				"run importbootenv; " \
			"fi;" \
			"if run loadimage; then " \
				"run mmcboot; " \
			"else " \
				"run netboot; " \
			"fi; " \
		"fi; " \
	"fi;"

/* Link Definitions */
#define CFG_SYS_INIT_RAM_ADDR        0x40000000
#define CFG_SYS_INIT_RAM_SIZE        0x200000

/* DDR configs */
#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define DEFAULT_SDRAM_SIZE		(512 * SZ_1M) /* 512MB Minimum DDR4, see get_dram_size */
#define VAR_EEPROM_DRAM_START          (CONFIG_SYS_MEMTEST_START + \
					(DEFAULT_SDRAM_SIZE >> 1))

#define CFG_MXC_UART_BASE		UART1_BASE_ADDR
#define CFG_MXC_UART_BASE_2		UART4_BASE_ADDR

/* USDHC */
#define CFG_SYS_FSL_USDHC_NUM	2
#define CFG_SYS_FSL_ESDHC_ADDR	0

/* USB configs */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USBD_HS
#endif

#define CONFIG_USB_GADGET_VBUS_DRAW 2

#define CONFIG_MXC_USB_PORTSC  (PORT_PTS_UTMI | PORT_PTS_PTW)

/* Carrier board EEPROM */
#define CARRIER_EEPROM_BUS_SOM		0x02
#define CARRIER_EEPROM_BUS_DART		0x01
#define CARRIER_EEPROM_ADDR		0x54

/* Define the offset for the FDT FIT hash */
#define CFG_FIT_FDT_HASH_OFFSET 0x23800

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx8mm_var_dart_android.h"
#endif

#endif
