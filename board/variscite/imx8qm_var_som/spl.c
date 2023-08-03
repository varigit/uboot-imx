/*
 * Copyright 2019-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <env.h>
#include <dm.h>
#include <i2c.h>
#include <spl.h>
#include <image.h>
#include <init.h>
#include <log.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <bootm.h>
#include <asm/gpio.h>
#include <asm/arch/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <asm/arch/iomux.h>

#include "../common/imx8_eeprom.h"
#include "imx8qm_var_som.h"

DECLARE_GLOBAL_DATA_PTR;

static struct var_eeprom eeprom = {0};

void spl_board_init(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	struct udevice *dev;
	int node, ret;

	node = fdt_node_offset_by_compatible(gd->fdt_blob, -1, "fsl,imx8-mu");

	ret = uclass_get_device_by_of_offset(UCLASS_MISC, node, &dev);
	if (ret) {
		return;
	}
	device_probe(dev);

	uclass_find_first_device(UCLASS_MISC, &dev);
	for (; dev; uclass_find_next_device(&dev)) {
		if (device_probe(dev))
			continue;
	}

	board_early_init_f();

	timer_init();

#ifdef CONFIG_SPL_SERIAL_SUPPORT
	preloader_console_init();
#endif

	memset(ep, 0, sizeof(*ep));
	if (!var_scu_eeprom_read_header(&eeprom))
		/* Copy EEPROM contents to DRAM */
		memcpy(ep, &eeprom, sizeof(*ep));

#ifdef CONFIG_SPL_SERIAL_SUPPORT
	puts("Normal Boot\n");
#endif
}

void spl_board_prepare_for_boot(void)
{
	board_quiesce_devices();
}

int board_fit_config_name_match(const char *name)
{
	return 0;
}

void board_init_f(ulong dummy)
{
	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	board_init_r(NULL, 0);
}
