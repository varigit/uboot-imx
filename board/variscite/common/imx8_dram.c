// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 * Copyright 2020 Variscite Ltd.
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include "imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_IMX8M_BOARD_INIT_DRAM

#define PHYS_SDRAM_LOW_MAX_ADDR		0x100000000ULL
#define PHYS_SDRAM_LOW_MAX_SIZE		(PHYS_SDRAM_LOW_MAX_ADDR - CONFIG_SYS_SDRAM_BASE)

static int get_dram_size(phys_size_t *size)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;

	if (!size)
		return -EINVAL;

	var_eeprom_get_dram_size(ep, size);

	return 0;
}

int dram_init_banksize(void)
{
	int bank = 0;
	int ret;
	phys_size_t low_mem_size, sdram_size;

	ret = get_dram_size(&sdram_size);
	if (ret)
		return ret;

	if (sdram_size >= PHYS_SDRAM_LOW_MAX_ADDR)
		low_mem_size = PHYS_SDRAM_LOW_MAX_SIZE;
	else
		low_mem_size = sdram_size;

	gd->bd->bi_dram[bank].start = PHYS_SDRAM;
	if (rom_pointer[1]) {
		phys_addr_t optee_start = (phys_addr_t)rom_pointer[0];
		phys_size_t optee_size = (size_t)rom_pointer[1];

		gd->bd->bi_dram[bank].size = optee_start -gd->bd->bi_dram[bank].start;
		if ((optee_start + optee_size) < (PHYS_SDRAM + low_mem_size)) {
			if ( ++bank >= CONFIG_NR_DRAM_BANKS) {
				puts("CONFIG_NR_DRAM_BANKS is not enough\n");
				return -1;
			}

			gd->bd->bi_dram[bank].start = optee_start + optee_size;
			gd->bd->bi_dram[bank].size = PHYS_SDRAM +
				low_mem_size - gd->bd->bi_dram[bank].start;
		}
	} else {
		gd->bd->bi_dram[bank].size = low_mem_size;
		debug("dram_init_banksize: bank[%d].start=0x%llx bank[%d].size=0x%llx\n",
			bank, gd->bd->bi_dram[bank].start, bank, gd->bd->bi_dram[bank].size);
	}

	if (sdram_size > PHYS_SDRAM_LOW_MAX_SIZE) {
		if ( ++bank >= CONFIG_NR_DRAM_BANKS) {
			puts("CONFIG_NR_DRAM_BANKS is not enough for SDRAM_2\n");
			return -1;
		}
		gd->bd->bi_dram[bank].start = PHYS_SDRAM_LOW_MAX_ADDR;
		gd->bd->bi_dram[bank].size = (sdram_size - PHYS_SDRAM_LOW_MAX_SIZE);
		debug("dram_init_banksize: bank[%d].start=0x%llx bank[%d].size=0x%llx\n",
			bank, gd->bd->bi_dram[bank].start, bank, gd->bd->bi_dram[bank].size);
	}

	return 0;
}

int dram_init(void)
{
	int ret;
	phys_size_t low_mem_size, sdram_size;

	ret = get_dram_size(&sdram_size);
	if (ret)
		return ret;

	if (sdram_size >= PHYS_SDRAM_LOW_MAX_ADDR)
		low_mem_size = PHYS_SDRAM_LOW_MAX_SIZE;
	else
		low_mem_size = sdram_size;

	/* rom_pointer[1] contains the size of TEE occupies */
	if (rom_pointer[1])
		gd->ram_size = low_mem_size - rom_pointer[1];
	else
		gd->ram_size = low_mem_size;

	if (sdram_size > PHYS_SDRAM_LOW_MAX_SIZE)
		gd->ram_size += (sdram_size - PHYS_SDRAM_LOW_MAX_SIZE);

	debug("dram_init: gd->ram_size = 0x%llx\n", gd->ram_size);

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	/* return the first bank as effective memory */
	if (rom_pointer[1])
		return ((phys_addr_t)rom_pointer[0] - PHYS_SDRAM);

	if (gd->ram_size > PHYS_SDRAM_LOW_MAX_SIZE) {
		debug("get_effective_memsize: size=0x%llx\n", PHYS_SDRAM_LOW_MAX_SIZE);
		return PHYS_SDRAM_LOW_MAX_SIZE;
	}
	else {
		debug("get_effective_memsize: size=0x%llx\n", gd->ram_size);
		return gd->ram_size;
	}
}
#endif
