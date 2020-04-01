/*
 * Copyright 2017-2019 NXP
 * Copyright 2019-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX8QXP_VAR_SOM_H
#define __IMX8QXP_VAR_SOM_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>

#include "imx_env.h"

#ifdef CONFIG_SPL_BUILD

#ifdef CONFIG_SPL_SPI_SUPPORT
#define CONFIG_SPL_SPI_LOAD
#endif

#define CONFIG_PARSE_CONTAINER
#define CONFIG_SPL_TEXT_BASE		0x0
#define CONFIG_SPL_MAX_SIZE		(124 * 1024)
#define CONFIG_SYS_MONITOR_LEN		(1024 * 1024)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR		0x1040 /* (32K + 2Mb)/sector_size */
#define CONFIG_SYS_SPI_U_BOOT_OFFS 0x200000

/*
 * 0x08081000 - 0x08180FFF is for m4_0 xip image,
  * So 3rd container image may start from 0x8181000
 */
#define CONFIG_SYS_UBOOT_BASE 			0x08181000
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	0


#define CONFIG_SPL_WATCHDOG_SUPPORT
#define CONFIG_SPL_DRIVERS_MISC_SUPPORT
#define CONFIG_SPL_LDSCRIPT		"arch/arm/cpu/armv8/u-boot-spl.lds"
#define CONFIG_SPL_STACK		0x013E000
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_BSS_START_ADDR	0x00138000
#define CONFIG_SPL_BSS_MAX_SIZE		0x1000	/* 4 KB */
#define CONFIG_SYS_SPL_MALLOC_START	0x00120000
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x18000	/* 12 KB */
#define CONFIG_SERIAL_LPUART_BASE	0x5a090000 /* UART3 */
#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_MALLOC_F_ADDR		0x00120000 /* malloc f used before GD_FLG_FULL_MALLOC_INIT set */

#define CONFIG_SPL_RAW_IMAGE_ARM_TRUSTED_FIRMWARE

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE /* For RAW image gives a error info not panic */

#define CONFIG_OF_EMBED
#define CONFIG_ATF_TEXT_BASE		0x80000000
#define CONFIG_SYS_ATF_START		0x80000000
/* #define CONFIG_FIT */

/* Since the SPL runs before ATF, MU1 will not be started yet, so use MU0 */
#define SC_IPC_CH			SC_IPC_AP_CH0

#endif

#define CONFIG_REMAKE_ELF

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_ARCH_MISC_INIT

#define CONFIG_CMD_READ

/* Flat Device Tree Definitions */
#define CONFIG_OF_BOARD_SETUP

#undef CONFIG_CMD_EXPORTENV
#undef CONFIG_CMD_IMPORTENV
#undef CONFIG_CMD_IMLS

#undef CONFIG_CMD_CRC32
#undef CONFIG_BOOTM_NETBSD

#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	0
#define USDHC1_BASE_ADDR		0x5B010000
#define USDHC2_BASE_ADDR		0x5B020000
#define CONFIG_SUPPORT_EMMC_BOOT	/* eMMC specific */

#define CONFIG_ENV_OVERWRITE

#undef CONFIG_FSL_HSIO
#ifdef CONFIG_FSL_HSIO
#define CONFIG_PCIE_IMX8X
#define CONFIG_CMD_PCI
#define CONFIG_PCI
#define CONFIG_PCI_PNP
#define CONFIG_PCI_SCAN_SHOW
#endif

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

/* FUSE command */
#define CONFIG_CMD_FUSE

/* GPIO configs */
#define CONFIG_MXC_GPIO

/* ENET Config */
#define CONFIG_MII

#define CONFIG_FEC_MXC
#define CONFIG_FEC_XCV_TYPE		RGMII
#define FEC_QUIRK_ENET_MAC

#define CONFIG_PHY_GIGE
#define CONFIG_PHYLIB
#define CONFIG_PHY_ATHEROS

#define CONFIG_LIB_RAND
#define CONFIG_NET_RANDOM_ETHADDR

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV	"sec_boot=yes\0"
#else
#define AHAB_ENV	"sec_boot=no\0"
#endif

/* Boot M4 */
#define M4_BOOT_ENV \
	"m4_0_image=m4_0.bin\0" \
	"loadm4image_0=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4_0_image}\0" \
	"m4boot_0=run loadm4image_0; dcache flush; bootaux ${loadaddr} 0\0" \

#define CONFIG_MFG_ENV_SETTINGS \
	CONFIG_MFG_ENV_SETTINGS_DEFAULT \
	"initrd_addr=0x83100000\0" \
	"initrd_high=0xffffffffffffffff\0" \
	"emmc_dev=0\0" \
	"sd_dev=1\0" \

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	CONFIG_MFG_ENV_SETTINGS \
	M4_BOOT_ENV \
	AHAB_ENV \
	"bootdir=/boot\0"	\
	"script=boot.scr\0" \
	"image=Image.gz\0" \
	"panel=VAR-WVGA-LCD\0" \
	"console=ttyLP3\0" \
	"img_addr=0x82000000\0"			\
	"fdt_addr=0x83000000\0"			\
	"fdt_high=0xffffffffffffffff\0"		\
	"cntr_addr=0x98000000\0"			\
	"cntr_file=os_cntr_signed.bin\0" \
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"fdt_file=fsl-imx8qxp-var-som.dtb\0" \
	"mmcdev="__stringify(CONFIG_SYS_MMC_ENV_DEV)"\0" \
	"mmcblk=1\0" \
	"mmcautodetect=yes\0" \
	"mmcpart=1\0" \
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} earlycon " \
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootfstype=ext4 rootwait rw\0 " \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		  "unzip ${img_addr} ${loadaddr}\0" \
	"loadfdt=load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
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
		"root=/dev/nfs " \
		"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0" \
	"netboot=echo Booting from net ...; " \
		"if test ${ip_dyn} = yes; then " \
			"setenv get_cmd dhcp; " \
		"else " \
			"setenv get_cmd tftp; " \
		"fi; " \
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

#define CONFIG_BOOTCOMMAND \
	   "mmc dev ${mmcdev}; if mmc rescan; then " \
		   "if run loadbootscript; then " \
			   "run bootscript; " \
		   "else " \
			   "if test ${sec_boot} = yes; then " \
				   "if run loadcntr; then " \
					   "run mmcboot; " \
				   "else run netboot; " \
				   "fi; " \
			    "else " \
				   "if run loadimage; then " \
					   "run mmcboot; " \
				   "else run netboot; " \
				   "fi; " \
			 "fi; " \
		   "fi; " \
	   "else booti ${loadaddr} - ${fdt_addr}; fi"

/* Link Definitions */
#define CONFIG_LOADADDR			0x80280000

#define CONFIG_SYS_LOAD_ADDR		CONFIG_LOADADDR

#define CONFIG_SYS_INIT_SP_ADDR		0x80200000

/* Default environment is in SD */
#define CONFIG_ENV_SIZE			0x2000
#define CONFIG_ENV_OFFSET		(64 * SZ_64K)
#define CONFIG_SYS_MMC_ENV_PART		0	/* user area */

/* USDHC1 is for eMMC, USDHC2 is for SD */
#define CONFIG_SYS_MMC_ENV_DEV		1   /* USDHC2 */
#define CONFIG_SYS_FSL_USDHC_NUM	2

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		((CONFIG_ENV_SIZE + (32*1024)) * 1024)

#define CONFIG_SYS_SDRAM_BASE		0x80000000
#define CONFIG_NR_DRAM_BANKS		4
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_2			0x880000000
#define DEFAULT_DRAM_SIZE_MB		2048

#define CONFIG_SYS_MEMTEST_START	0xA0000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + ((DEFAULT_DRAM_SIZE_MB * 1024) >> 2))

/* Serial */
#define CONFIG_BAUDRATE			115200

/* Monitor Command Prompt */
#define CONFIG_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_CBSIZE		2048
#define CONFIG_SYS_MAXARGS		64
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY		8000000	/* 8MHz */

#ifndef CONFIG_DM_PCA953X
#define CONFIG_PCA953X
#define CONFIG_CMD_PCA953X
#define CONFIG_CMD_PCA953X_INFO
#endif

#define CONFIG_IMX_SMMU

#define CONFIG_SERIAL_TAG

/* USB Config */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_CMD_USB
#define CONFIG_USB_STORAGE
#define CONFIG_USBD_HS

#define CONFIG_CMD_USB_MASS_STORAGE
#define CONFIG_USB_GADGET_MASS_STORAGE
#define CONFIG_USB_FUNCTION_MASS_STORAGE

#define CONFIG_USB_EHCI_HCD
#endif

#define CONFIG_USB_MAX_CONTROLLER_COUNT	2

/* USB OTG controller configs */
#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_EHCI_MX6
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC		(PORT_PTS_UTMI | PORT_PTS_PTW)
#endif

/* Framebuffer */
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_IMXDPUV1
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_BMP_LOGO
#define CONFIG_IMX_VIDEO_SKIP
#endif

/* SPLASH SCREEN Configs  */
#ifdef CONFIG_SPLASH_SCREEN
#define CONFIG_CMD_BMP
#define CONFIG_SPLASH_SCREEN_ALIGN
#define CONFIG_SPLASH_SOURCE
#endif

#define CONFIG_OF_SYSTEM_SETUP

#if defined(CONFIG_ANDROID_SUPPORT)
#include "imx8qxp_var_som_android.h"
#elif defined (CONFIG_ANDROID_AUTO_SUPPORT)
#include "imx8qxp_var_som_android_auto.h"
#endif

#endif /* __IMX8QXP_VAR_SOM_H */
