/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright 2017 NXP
 * Copyright 2020-2024 Variscite Ltd.
 *
 */

#ifndef __IMX8MQ_VAR_DART_H
#define __IMX8MQ_VAR_DART_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#ifdef CONFIG_SPL_BUILD
/*#define CONFIG_ENABLE_DDR_TRAINING_DEBUG*/

/* malloc f used before GD_FLG_FULL_MALLOC_INIT set */
#define CFG_MALLOC_F_ADDR		0x182000

#define CFG_POWER_PFUZE100_I2C_ADDR 0x08
#endif

#ifdef CONFIG_DISTRO_DEFAULTS
#define BOOT_TARGET_DEVICES(func) \
	func(USB, usb, 0) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0)

#include <config_distro_bootcmd.h>
/* redefine BOOTENV_EFI_SET_FDTFILE_FALLBACK to use Variscite function to load fdt */
#undef BOOTENV_EFI_SET_FDTFILE_FALLBACK
#define BOOTENV_EFI_SET_FDTFILE_FALLBACK \
	"setenv efi_dtb_prefixes; " \
	"run loadfdt; "
#else
#define BOOTENV
#endif

/* UUU environment variables */
#define CFG_MFG_ENV_SETTINGS \
	CFG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0"\
	"sd_dev=1\0" \

/* Initial environment variables */
#define CFG_EXTRA_ENV_SETTINGS		\
	CFG_MFG_ENV_SETTINGS \
	"bootdir=/boot\0"	\
	BOOTENV \
	"scriptaddr=0x43500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"bsp_script=boot.scr\0" \
	"image=Image.gz\0" \
	"img_addr=0x42000000\0"			\
	"console=ttymxc0,115200 earlycon=ec_imx6q,0x30860000,115200\0" \
	"fdt_addr_r=0x43000000\0" \
	"fdt_addr=0x43000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"fdt_file=undefined\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=1\0" \
	"mmcautodetect=yes\0" \
	"mmcpart=1\0" \
	"m4_addr=0x7e0000\0" \
	"m4_bin=hello_world.bin\0" \
	"use_m4=no\0" \
	"dfu_alt_info=mmc 2=1 raw 0x42 0x1000 mmcpart\0" \
	"loadm4bin=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${m4_bin} && " \
	"cp.b ${loadaddr} ${m4_addr} ${filesize}\0" \
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
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootwait rw ${cma_size} cma_name=linux,cma\0" \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${bsp_script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		"unzip ${img_addr} ${loadaddr}\0" \
	"ramsize_check="\
		"if test $sdram_size -le 512; then " \
			"setenv cma_size cma=320M; " \
		"else " \
			"if test $sdram_size -le 1024; then " \
				"if test ${use_m4} = yes; then " \
					"setenv cma_size cma=576M; " \
				"else " \
					"setenv cma_size cma=608M; " \
				"fi; " \
			"else " \
				"if test ${use_m4} = yes; then " \
					"setenv cma_size cma=928M; " \
				"else " \
					"setenv cma_size cma=960M; " \
				"fi; " \
			"fi; " \
		"fi;\0" \
	"findfdt=" \
		"if test $fdt_file = undefined; then " \
			"setenv fdt_file imx8mq-var-dart-dt8mcustomboard.dtb; " \
		"fi; \0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcpart} ${fdt_addr_r} ${bootdir}/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"run optargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"booti ${loadaddr} - ${fdt_addr_r}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"else " \
			"echo wait for boot; " \
		"fi;\0" \
	"netargs=setenv bootargs console=${console} " \
		"root=/dev/nfs ${cma_size} cma_name=linux,cma " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run netargs; " \
		"run optargs; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${img_addr} ${image}; unzip ${img_addr} ${loadaddr};" \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"run findfdt; " \
			"echo fdt_file=${fdt_file}; " \
			"if ${get_cmd} ${fdt_addr_r} ${fdt_file}; then " \
				"booti ${loadaddr} - ${fdt_addr_r}; " \
			"else " \
				"echo WARN: Cannot load the DT; " \
			"fi; " \
		"else " \
			"booti; " \
		"fi;\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x43100000\0" \
	"splashenable=setenv splashfile /boot/splash.bmp; " \
	"setenv splashimage 0x43100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0" \
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
			"if run loadimage; then " \
				"run mmcboot; " \
			"else " \
				"run netboot; " \
			"fi; " \
		"fi; " \
	"fi;"

/* Link Definitions */

#define CFG_SYS_INIT_RAM_ADDR        0x40000000
#define CFG_SYS_INIT_RAM_SIZE        0x80000

#define CFG_SYS_SDRAM_BASE           0x40000000
#define PHYS_SDRAM                      0x40000000
#define DEFAULT_SDRAM_SIZE		(512 * SZ_1M) /* 512MB Minimum DDR4, see get_dram_size */
#define VAR_EEPROM_DRAM_START          (CONFIG_SYS_MEMTEST_START + \
					(DEFAULT_SDRAM_SIZE >> 1))

#define CFG_MXC_UART_BASE		UART1_BASE_ADDR

#define CFG_SYS_FSL_USDHC_NUM	2
#define CFG_SYS_FSL_ESDHC_ADDR       0

/* USB configs */
#define CONFIG_SERIAL_TAG
#define CONFIG_FASTBOOT_USB_DEV 0

#define CONFIG_USB_MAX_CONTROLLER_COUNT         2
#define CONFIG_USB_GADGET_VBUS_DRAW 2

#endif
