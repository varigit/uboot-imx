/*
 * Copyright 2017-2018 NXP
 * Copyright 2019 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX8QM_VAR_SOM_H
#define __IMX8QM_VAR_SOM_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>
#include "imx_env.h"

#ifdef CONFIG_SPL_BUILD
#define CONFIG_PARSE_CONTAINER
#define CONFIG_SPL_TEXT_BASE				0x100000
#define CONFIG_SPL_MAX_SIZE				(192 * 1024)
#define CONFIG_SYS_MONITOR_LEN				(1024 * 1024)
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_USE_SECTOR
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR		0x1040 /* (flash.bin_offset + 2Mb)/sector_size */
#define CONFIG_SYS_SPI_U_BOOT_OFFS			0x200000

/*
 * 0x08081000 - 0x08180FFF is for m4_0 xip image,
 * 0x08181000 - 0x008280FFF is for m4_1 xip image
  * So 3rd container image may start from 0x8281000
 */
#define CONFIG_SYS_UBOOT_BASE				0x08281000
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION		0

#define CONFIG_SPL_LDSCRIPT				"arch/arm/cpu/armv8/u-boot-spl.lds"
/*
 * The memory layout on stack:  DATA section save + gd + early malloc
 * the idea is re-use the early malloc (CONFIG_SYS_MALLOC_F_LEN) with
 * CONFIG_SYS_SPL_MALLOC_START
 */
#define CONFIG_SPL_STACK				0x013E000
#define CONFIG_SPL_BSS_START_ADDR			0x00130000
#define CONFIG_SPL_BSS_MAX_SIZE				0x1000  /* 4 KB */
#define CONFIG_SYS_SPL_MALLOC_START			0x82200000			//tobe verified
#define CONFIG_SYS_SPL_MALLOC_SIZE			0x80000  /* 512 KB */		//tobe verified
#define CONFIG_SERIAL_LPUART_BASE			0x5a060000
#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF
#define CONFIG_MALLOC_F_ADDR				0x00138000			//tobe verified

#define CONFIG_SPL_RAW_IMAGE_ARM_TRUSTED_FIRMWARE

#define CONFIG_SPL_ABORT_ON_RAW_IMAGE /* For RAW image gives a error info not panic */

#define CONFIG_OF_EMBED
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
#define CONFIG_SYS_FSL_ESDHC_ADDR			0
#define USDHC1_BASE_ADDR				0x5B010000
#define USDHC2_BASE_ADDR				0x5B020000

#define CONFIG_SUPPORT_EMMC_BOOT   /* eMMC specific */

#define CONFIG_ENV_OVERWRITE

#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG

#define CONFIG_FEC_XCV_TYPE				RGMII
#define FEC_QUIRK_ENET_MAC

#ifndef CONFIG_LIB_RAND
#define CONFIG_LIB_RAND
#endif
#define CONFIG_NET_RANDOM_ETHADDR

#ifdef CONFIG_AHAB_BOOT
#define AHAB_ENV					"sec_boot=yes\0"
#else
#define AHAB_ENV					"sec_boot=no\0"
#endif

#define JAILHOUSE_ENV \
	"jh_mmcboot=" \
		"setenv fdt_file imx8qm-var-som-root.dtb;"\
		"setenv boot_os 'scu_rm dtb ${fdt_addr}; booti ${loadaddr} - ${fdt_addr};'; " \
		"run mmcboot; \0" \
	"jh_netboot=" \
		"setenv fdt_file imx8qm-var-som-root.dtb;"\
		"setenv boot_os 'scu_rm dtb ${fdt_addr}; booti ${loadaddr} - ${fdt_addr};'; " \
		"run netboot; \0"

#define XEN_BOOT_ENV \
	    "domu-android-auto=no\0" \
            "xenhyper_bootargs=console=dtuart dtuart=/serial@5a060000 dom0_mem=2048M dom0_max_vcpus=2 dom0_vcpus_pin=true hmp-unsafe=true\0" \
            "xenlinux_bootargs= \0" \
            "xenlinux_console=hvc0 earlycon=xen\0" \
            "xenlinux_addr=0x9e000000\0" \
	    "dom0fdt_file=imx8qm-var-som-dom0.dtb\0" \
            "xenboot_common=" \
                "${get_cmd} ${loadaddr} xen;" \
                "${get_cmd} ${fdt_addr} ${dom0fdt_file};" \
                "if ${get_cmd} ${hdp_addr} ${hdp_file}; then; hdp load ${hdp_addr}; fi;" \
				"if ${get_cmd} ${hdprx_addr} ${hdprx_file}; then; hdprx load ${hdprx_addr}; fi;" \
                "${get_cmd} ${xenlinux_addr} ${image};" \
                "fdt addr ${fdt_addr};" \
                "fdt resize 256;" \
                "fdt set /chosen/module@0 reg <0x00000000 ${xenlinux_addr} 0x00000000 0x${filesize}>; " \
                "fdt set /chosen/module@0 bootargs \"${bootargs} ${xenlinux_bootargs}\"; " \
		"if test ${domu-android-auto} = yes; then; " \
			"fdt set /domu/doma android-auto <1>;" \
			"fdt rm /gpio@5d090000 power-domains;" \
		"fi;" \
                "setenv bootargs ${xenhyper_bootargs};" \
                "booti ${loadaddr} - ${fdt_addr};" \
            "\0" \
            "xennetboot=" \
                "setenv get_cmd dhcp;" \
                "setenv console ${xenlinux_console};" \
                "run netargs;" \
                "run xenboot_common;" \
            "\0" \
            "xenmmcboot=" \
                "setenv get_cmd \"load mmc ${mmcdev}:${mmcpart}\";" \
                "setenv console ${xenlinux_console};" \
                "run mmcargs;" \
                "run xenboot_common;" \
            "\0" \

/* Boot M4 */
#define M4_BOOT_ENV \
	"m4_0_image=m4_0.bin\0" \
	"m4_1_image=m4_1.bin\0" \
	"loadm4image_0=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4_0_image}\0" \
	"loadm4image_1=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${m4_1_image}\0" \
	"m4boot_0=run loadm4image_0; dcache flush; bootaux ${loadaddr} 0\0" \
	"m4boot_1=run loadm4image_1; dcache flush; bootaux ${loadaddr} 1\0" \

#ifdef CONFIG_NAND_BOOT
#define MFG_NAND_PARTITION "mtdparts=gpmi-nand:128m(boot),32m(kernel),16m(dtb),8m(misc),-(rootfs) "
#else
#define MFG_NAND_PARTITION ""
#endif

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
	XEN_BOOT_ENV \
	JAILHOUSE_ENV\
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
	"optargs=setenv bootargs ${bootargs} ${kernelargs};\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} earlycon " \
		"root=/dev/mmcblk${mmcblk}p${mmcpart} rootfstype=ext4 rootwait rw\0 " \
	"loadbootscript=load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"loadimage=load mmc ${mmcdev}:${mmcpart} ${img_addr} ${bootdir}/${image};" \
		"unzip ${img_addr} ${loadaddr}\0" \
	"findfdt=" \
		"if test $fdt_file = undefined; then " \
			"if test $board_name = VAR-SOM-MX8; then " \
				"setenv fdt_file imx8qm-var-som.dtb; " \
			"else " \
				"setenv fdt_file imx8qm-var-spear.dtb;" \
			"fi; " \
		"fi; \0" \
	"loadfdt=run findfdt; " \
		"echo fdt_file=${fdt_file}; " \
		"load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${bootdir}/${fdt_file}\0" \
	"hdp_addr=0x9c000000\0" \
	"hdprx_addr=0x9c800000\0" \
	"hdp_file=hdmitxfw.bin\0" \
	"hdprx_file=hdmirxfw.bin\0" \
	"loadhdp=load mmc ${mmcdev}:${mmcpart} ${hdp_addr} ${bootdir}/${hdp_file}\0" \
	"loadhdprx=load mmc ${mmcdev}:${mmcpart} ${hdprx_addr} ${bootdir}/${hdprx_file}\0" \
	"boot_os=booti ${loadaddr} - ${fdt_addr};\0" \
	"loadcntr=load mmc ${mmcdev}:${mmcpart} ${cntr_addr} ${bootdir}/${cntr_file}\0" \
	"auth_os=auth_cntr ${cntr_addr}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"if run loadhdp; then; hdp load ${hdp_addr}; fi;" \
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
#define CONFIG_LOADADDR					0x80280000

#define CONFIG_SYS_LOAD_ADDR				CONFIG_LOADADDR

#define CONFIG_SYS_INIT_SP_ADDR				0x80200000

/* Default environment is in SD */
#define CONFIG_ENV_SIZE					0x2000

#define CONFIG_ENV_OFFSET				(64 * SZ_64K)
#define CONFIG_SYS_MMC_ENV_PART				0	/* user area */

#define CONFIG_SYS_MMC_ENV_DEV				1   /* USDHC1 */
#define CONFIG_SYS_FSL_USDHC_NUM			2

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN				((CONFIG_ENV_SIZE + (32*1024)) * 1024)

#define CONFIG_SYS_SDRAM_BASE				0x80000000
#define PHYS_SDRAM_1					0x80000000
#define PHYS_SDRAM_2					0x880000000
#define DEFAULT_DRAM_SIZE_MB				4096

#define CONFIG_SYS_MEMTEST_START			0xA0000000
#define CONFIG_SYS_MEMTEST_END				(CONFIG_SYS_MEMTEST_START + ((DEFAULT_DRAM_SIZE_MB * 1024) >> 2))

/* Serial */
#define CONFIG_BAUDRATE					115200

/* Monitor Command Prompt */
#define CONFIG_SYS_PROMPT_HUSH_PS2			"> "
#define CONFIG_SYS_CBSIZE				2048
#define CONFIG_SYS_MAXARGS				64
#define CONFIG_SYS_BARGSIZE				CONFIG_SYS_CBSIZE
#define CONFIG_SYS_PBSIZE				(CONFIG_SYS_CBSIZE + \
								sizeof(CONFIG_SYS_PROMPT) + 16)

/* Generic Timer Definitions */
#define COUNTER_FREQUENCY				8000000	/* 8MHz */

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
#endif

#define CONFIG_USB_MAX_CONTROLLER_COUNT			2

/* USB OTG controller configs */
#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_HOST_ETHER
#define CONFIG_USB_ETHER_ASIX
#define CONFIG_MXC_USB_PORTSC				(PORT_PTS_UTMI | PORT_PTS_PTW)
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
#include "imx8qm_var_som_android.h"
#endif

#endif /* __IMX8QM_VAR_SOM_H */
