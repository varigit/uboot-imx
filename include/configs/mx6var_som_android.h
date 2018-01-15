/*
 * Copyright (C) 2017 Variscite Ltd.
 */

#ifndef __MX6VAR_SOM_ANDROID_H
#define __MX6VAR_SOM_ANDROID_H

#include "mx_var_som_android_common.h"

/*#define CONFIG_FASTBOOT_LOCK*/ /*Unlock for dev Devices*/
#define FSL_FASTBOOT_FB_DEV "mmc"
/*#define FASTBOOT_ENCRYPT_LOCK*/ /*Don't encrypt fastboot lock state*/
#define CONFIG_FSL_CAAM_KB
#define CONFIG_CMD_FSL_CAAM_KB
#define CONFIG_SHA1
#define CONFIG_SHA256

#define HW_ENV_SETTINGS \
	"hwargs=" \
		"if test $board_som = DART-MX6; then " \
			"setenv hardware var-dart-mx6; " \
		"else " \
			"setenv hardware var-som-mx6; " \
		"fi; " \
		"setenv bootargs ${bootargs} " \
			"androidboot.hardware=${hardware};\0"

#define BOOT_ENV_SETTINGS \
	"bootcmd=" \
		"run hwargs; " \
		"run videoargs; " \
		"boota ${boota_dev}\0" \
	"bootcmd_android_recovery=" \
		"run hwargs; " \
		"run videoargs; " \
		"boota ${recovery_dev} recovery\0" \
	"fastboot_dev=mmc1\0" \
	"boota_dev=mmc1\0" \
	"recovery_dev=mmc1\0" \
	"dev_autodetect=yes\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	HW_ENV_SETTINGS \
	VIDEO_ENV_SETTINGS \
	"splashpos=m,m\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"bootargs=" \
		"console=ttymxc0,115200 " \
		"init=/init " \
		"vmalloc=128M " \
		"androidboot.console=ttymxc0 " \
		"consoleblank=0 " \
		"cma=448M " \
		"firmware_class.path=/system/etc/firmware\0"

#endif	/* __MX6VAR_SOM_ANDROID_H */
