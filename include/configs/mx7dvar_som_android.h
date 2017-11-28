
/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 * Copyright (C) 2016-2017 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7D_VAR_SOM_ANDROID_H
#define __MX7D_VAR_SOM_ANDROID_H

#include "mx_var_som_android_common.h"

#define BOOT_ENV_SETTINGS \
	"bootcmd=" \
		"boota ${boota_dev}\0" \
	"bootcmd_android_recovery=" \
		"boota ${recovery_dev} recovery\0" \
	"fastboot_dev=mmc1\0" \
	"boota_dev=mmc1\0" \
	"recovery_dev=mmc1\0" \
	"dev_autodetect=yes\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	BOOT_ENV_SETTINGS \
	"splashpos=m,m\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"bootargs=" \
		"console=ttymxc0,115200 " \
		"init=/init " \
		"androidboot.console=ttymxc0 " \
		"androidboot.hardware=var-som-mx7 " \
		"consoleblank=0 " \
		"firmware_class.path=/system/etc/firmware\0"

#endif	/* __MX7D_VAR_SOM_ANDROID_H */
