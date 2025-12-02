/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 * Copyright (C) 2019 Parthiban Nallathambi <parthitce@gmail.com>
 */

#ifndef __IMX6UL_VAR_DART_H
#define __IMX6UL_VAR_DART_H

#include <linux/sizes.h>
#include <linux/stringify.h>
#include "mx6_common.h"

/* NAND pin conflicts with usdhc2 */
#ifdef CONFIG_CMD_NAND
#define CFG_SYS_FSL_USDHC_NUM        1
#else
#define CFG_SYS_FSL_USDHC_NUM        2
#endif

#ifdef CONFIG_CMD_NET
#define CFG_FEC_ENET_DEV		0
#endif

/* Environment settings */

/* Environment in SD */
#define MMC_ROOTFS_DEV			0
#define MMC_ROOTFS_PART			2

/* Console configs */
#define CFG_MXC_UART_BASE		UART1_BASE

/* MMC Configs */

#define CFG_SYS_FSL_ESDHC_ADDR	USDHC2_BASE_ADDR

/* I2C configs */

/* Miscellaneous configurable options */

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR
#define PHYS_MIN_SDRAM_SIZE			SZ_128M
#define VAR_EEPROM_DRAM_START          (PHYS_SDRAM + (PHYS_MIN_SDRAM_SIZE >> 1))

#define CFG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CFG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CFG_SYS_INIT_RAM_SIZE	IRAM_SIZE

/* USB Configs */
#define CFG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#define CFG_MXC_USB_FLAGS		0

#define NAND_BOOT_ENV_SETTINGS \
	"nandargs=setenv bootargs console=${console},${baudrate} " \
		"ubi.mtd=4 root=ubi0:rootfs rootfstype=ubifs rw ${cma_size}\0" \
	"nandboot=echo Booting from nand ...; " \
		"run ramsize_check; " \
		"run nandargs; " \
		"run optargs; " \
		"nand read ${loadaddr} 0x500000 0xbe0000; " \
		"nand read ${fdt_addr} 0x10e0000 0x20000; " \
		"bootz ${loadaddr} - ${fdt_addr}\0" \
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0" \
	CONFIG_MTDPARTS_DEFAULT "\0" \

#define MMC_BOOT_ENV_SETTINGS \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=0\0" \
	"mmcautodetect=yes\0" \
	"mmcbootpart=1\0" \
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
	"loadimage=load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootdir}/${image}\0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcbootpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run ramsize_check; " \
		"run mmcargs; " \
		"run optargs; " \
		"if test \"${boot_fdt}\" = yes || test \"${boot_fdt}\" = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test \"${boot_fdt}\" = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi\0" \

#ifdef CONFIG_NAND_BOOT
#define BOOT_ENV_SETTINGS	NAND_BOOT_ENV_SETTINGS
#else
#define BOOT_ENV_SETTINGS	MMC_BOOT_ENV_SETTINGS
#endif

#define OPT_ENV_SETTINGS \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0"

#define CFG_EXTRA_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	OPT_ENV_SETTINGS \
	"bootenv=uEnv.txt\0" \
	"script=boot.scr\0" \
	"image=zImage\0" \
	"console=ttymxc0\0" \
	"carrier=undefined\0" \
	"fdt_file=undefined\0" \
	"fdt_addr=0x83000000\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"nfsroot=/srv/nfs/" CONFIG_SYS_BOARD "/rootfs\0" \
	"panel=VAR-WVGA-LCD\0" \
	"splashsourceauto=yes\0" \
	"splashfile=/boot/splash.bmp\0" \
	"splashimage=0x83100000\0" \
	"splashenable=setenv splashfile /boot/splash.bmp; " \
		"setenv splashimage 0x83100000\0" \
	"splashdisable=setenv splashfile; setenv splashimage\0" \
	"splashpos=m,m\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"netargs=setenv bootargs console=${console},${baudrate} " \
		"root=/dev/nfs rw ${cma_size} " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"run ramsize_check; " \
		"run netargs; " \
		"run optargs; " \
		"if test \"${ip_dyn}\" = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
		"${get_cmd} ${image}; " \
		"if test \"${boot_fdt}\" = yes || test \"${boot_fdt}\" = try; then " \
			"run findfdt; " \
			"echo fdt_file=${fdt_file}; " \
			"if ${get_cmd} ${fdt_addr} ${fdt_file}; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test \"${boot_fdt}\" = try; then " \
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
		"if test \"${sdram_size}\" -lt 256; then " \
			"setenv cma_size cma=16MB; " \
			"setenv fdt_addr 0x83A00000; " \
			"setenv loadaddr 0x83E00000; " \
		"elif test \"${sdram_size}\" -lt 512; then " \
			"setenv cma_size cma=32MB; " \
		"else " \
			"setenv cma_size cma=64MB; " \
		"fi;\0" \
	"findfdt="\
		"if test \"${fdt_file}\" = undefined; then " \
			"if test \"${board_name}\" = DART-6UL; then " \
				"setenv som var-dart; " \
				"if test \"${carrier}\" = undefined; then " \
					"setenv carrier 6ulcustomboard; " \
				"fi; " \
			"fi; " \
			"if test \"${board_name}\" = VAR-SOM-6UL; then " \
				"setenv som var-som; " \
				"if test \"${carrier}\" = undefined; then " \
					"i2c dev 0; " \
					"if i2c probe 0x20; then " \
						"setenv carrier symphony-board; " \
					"else " \
						"setenv carrier concerto-board; " \
					"fi; " \
				"fi; " \
			"fi; " \
			"if test \"${boot_dev}\" = emmc || test \"${som_storage}\" = emmc || " \
			   "test \"${som_storage}\" = none; then " \
				"setenv storage emmc; " \
			"fi; " \
			"if test \"${boot_dev}\" = nand || test \"${som_storage}\" = nand; then " \
				"setenv storage nand; " \
			"fi; " \
			"if test \"${boot_dev}\" = sd; then " \
				"setenv mmc0_dev sd-card; " \
			"else " \
				"if test \"${wifi}\" = yes; then " \
					"if test \"${board_name}\" = VAR-SOM-6UL; then " \
						"setenv mmc0_dev wifi; " \
					"else " \
						"if test \"${som_rev}\" = '5G IW611' || test \"${som_rev}\" = '5G IW612'; then " \
							"setenv mmc0_dev wifi-iw61x; " \
						"else " \
							"setenv mmc0_dev wifi-brcm; " \
						"fi; " \
					"fi; " \
				"else " \
					"setenv mmc0_dev sd-card; " \
				"fi; " \
			"fi; " \
			"if test -n \"${soc_type}\" && test -n \"${som}\" && " \
			   "test -n \"${storage}\" && test -n \"${mmc0_dev}\" && test -n \"${carrier}\"; then " \
				"if test -n \"${codec}\" && test \"${codec}\" = wm8731; then " \
					"setenv fdt_file ${soc_type}-${som}-${carrier}-${storage}-${mmc0_dev}-${codec}.dtb; " \
				"else " \
					"setenv fdt_file ${soc_type}-${som}-${carrier}-${storage}-${mmc0_dev}.dtb; " \
				"fi; " \
			"fi; " \
			"setenv som; setenv carrier; setenv storage; setenv mmc0_dev; " \
			"if test \"${fdt_file}\" = undefined; then " \
				"echo WARNING: Could not determine dtb to use; " \
			"fi; " \
		"fi;\0"

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(DHCP, dhcp, na)

#include <config_distro_bootcmd.h>
#endif /* __IMX6UL_VAR_DART_H */
