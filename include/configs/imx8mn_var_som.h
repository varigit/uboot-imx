/*
 * Copyright 2018 NXP
 * Copyright 2019-2023 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX8MN_VAR_SOM_H
#define __IMX8MN_VAR_SOM_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#ifdef CONFIG_SECURE_BOOT
#define CONFIG_CSF_SIZE			0x2000 /* 8K region */
#endif

#define CONFIG_SYS_BOOTM_LEN		(32 * SZ_1M)
#define CONFIG_SPL_MAX_SIZE		(208 * 1024)
#define CONFIG_SYS_MONITOR_LEN		(512 * 1024)
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SYS_UBOOT_BASE		(QSPI0_AMBA_BASE + CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR * 512)

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SPL_STACK		0x95dff0
#define CONFIG_SPL_BSS_START_ADDR	0x95e000
#define CONFIG_SPL_BSS_MAX_SIZE	SZ_8K		/* 8 KB */
#define CONFIG_SYS_SPL_MALLOC_START	0x42200000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_512K	/* 512 KB */

/* For RAW image gives a error info not panic */
#define CONFIG_SPL_ABORT_ON_RAW_IMAGE

#endif /* CONFIG_SPL_BUILD */

#define CONFIG_CMD_READ
#define CONFIG_SERIAL_TAG
#define CONFIG_FASTBOOT_USB_DEV 0

#define CONFIG_REMAKE_ELF

/* ENET Config */
#if defined(CONFIG_FEC_MXC)
#define CONFIG_BOOTP_SERVERIP
#define CONFIG_ETHPRIME		"FEC"
#define PHY_ANEG_TIMEOUT		20000

#define CONFIG_FEC_XCV_TYPE		RGMII
#define FEC_QUIRK_ENET_MAC
#ifndef CONFIG_CMD_NET
#define CONFIG_CMD_NET
#endif
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

/* UUU environment variables */
#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x43800000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=2\0" \
	"sd_dev=1\0" \

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS \
	"bootdir=/boot\0" \
	BOOTENV \
	"prepare_mcore=setenv mcore_clk clk-imx8mn.mcore_booted;\0" \
	"scriptaddr=0x43500000\0" \
	"kernel_addr_r=" __stringify(CONFIG_SYS_LOAD_ADDR) "\0" \
	"bsp_script=boot.scr\0" \
	"image=Image.gz\0" \
	"img_addr=0x42000000\0" \
	"console=ttymxc3,115200 earlycon=ec_imx6q,0x30a60000,115200\0" \
	"fdt_addr_r=0x43000000\0" \
	"fdt_addr=0x43000000\0" \
	"fdt_high=0xffffffffffffffff\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"fdt_file=undefined\0" \
	"bootm_size=0x10000000\0" \
	"initrd_addr=0x43800000\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=1\0" \
	"mmcautodetect=yes\0" \
	"mmcpart=1\0" \
	"m7_addr=0x7e0000\0" \
	"m7_bin=hello_world.bin\0" \
	"use_m7=no\0" \
	"dfu_alt_info=mmc 2=1 raw 0x40 0x1000 mmcpart\0" \
	"loadm7bin=" \
		"load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${m7_bin} && " \
		"cp.b ${loadaddr} ${m7_addr} ${filesize}; " \
		"echo Init rsc_table region memory; " \
		"mw.b 400ff000 0 10\0" \
	"runm7bin=" \
		"if test ${m7_addr} = 0x7e0000; then " \
			"echo Booting M7 from TCM; " \
		"else " \
			"echo Booting M7 from DRAM; " \
			"dcache flush; " \
		"fi; " \
		"bootaux ${m7_addr};\0" \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs ${mcore_clk} console=${console} " \
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootwait rw ${cma_size} cma_name=linux,cma\0 " \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${bsp_script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		"unzip ${img_addr} ${loadaddr}\0" \
	"findfdt=" \
		"if test $fdt_file = undefined; then " \
			"setenv fdt_file imx8mn-var-som-symphony.dtb; " \
		"fi; \0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcpart} ${fdt_addr_r} ${bootdir}/${fdt_file}\0" \
	"ramsize_check="\
		"if test $sdram_size -le 512; then " \
			"setenv cma_size cma=320M; " \
		"else " \
			"setenv cma_size cma=576M; " \
		"fi;\0" \
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
	"netargs=setenv bootargs ${mcore_clk} console=${console} " \
		"root=/dev/nfs ${cma_size} cma_name=linux,cma " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${img_addr} ${image}; unzip ${img_addr} ${loadaddr};" \
		"run netargs; " \
		"run optargs; " \
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
	"bsp_bootcmd=echo Running BSP bootcmd ...; " \
	"run ramsize_check; " \
	"mmc dev ${mmcdev}; " \
	"if mmc rescan; then " \
		"if test ${use_m7} = yes && run loadm7bin; then " \
			"run runm7bin; " \
		"fi; " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else "\
			"if run loadimage; then " \
				"run mmcboot; " \
			"else " \
				"run netboot; " \
			"fi; " \
		"fi; " \
	"else " \
		"booti ${loadaddr} - ${fdt_addr}; " \
	"fi;"

/* Link Definitions */

#define CONFIG_SYS_INIT_RAM_ADDR	0x40000000
#define CONFIG_SYS_INIT_RAM_SIZE	0x80000
#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define PHYS_SDRAM			0x40000000
#define DEFAULT_SDRAM_SIZE 			(512 * SZ_1M) /* 512MB Minimum DDR4, see get_dram_size */
#define VAR_EEPROM_DRAM_START           (PHYS_SDRAM + (DEFAULT_SDRAM_SIZE >> 1))

#define CONFIG_BAUDRATE			115200

#define CONFIG_MXC_UART_BASE		UART4_BASE_ADDR

/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_IMX_BOOTAUX

/* USDHC configs */
#define CONFIG_SYS_FSL_USDHC_NUM	2
#define CONFIG_SYS_FSL_ESDHC_ADDR	0

#ifndef CONFIG_MXC_OCOTP
#define CONFIG_MXC_OCOTP
#endif

/* I2C configs */
#define CONFIG_SYS_I2C_SPEED		100000

/* USB configs */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USBD_HS
#endif

#define CONFIG_USB_GADGET_VBUS_DRAW 2

#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CONFIG_USB_MAX_CONTROLLER_COUNT	2

/* Carrier board EEPROM */
#define CARRIER_EEPROM_BUS		0x02
#define CARRIER_EEPROM_ADDR		0x54

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx8mn_var_som_android.h"
#endif

#endif
