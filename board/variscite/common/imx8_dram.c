// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 * Copyright 2020-2023 Variscite Ltd.
 */

#include <common.h>
#include <asm/arch/sys_proto.h>
#include <asm/global_data.h>
#include <asm/armv8/mmu.h>
#include "imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#if defined (CONFIG_IMX8M_BOARD_INIT_DRAM) || defined (CONFIG_IMX8_BOARD_INIT_DRAM)

static int get_dram_size(phys_size_t *size)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;

	if (!size)
		return -EINVAL;

	var_eeprom_get_dram_size(ep, size);

	return 0;
}
#endif

#ifdef CONFIG_IMX8M_BOARD_INIT_DRAM

#define PHYS_SDRAM_LOW_MAX_ADDR		0x100000000ULL
#define PHYS_SDRAM_LOW_MAX_SIZE		(PHYS_SDRAM_LOW_MAX_ADDR - CFG_SYS_SDRAM_BASE)

int dram_init_banksize(void)
{
	int bank = 0;
	int ret;
	phys_size_t low_mem_size, sdram_size;

	ret = get_dram_size(&sdram_size);
	if (ret)
		return ret;

	if (sdram_size >= PHYS_SDRAM_LOW_MAX_SIZE)
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

	if (sdram_size >= PHYS_SDRAM_LOW_MAX_SIZE)
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

#ifdef CONFIG_IMX8_BOARD_INIT_DRAM

#define MEMSTART_ALIGNMENT  SZ_2M /* Align the memory start with 2MB */

static int get_owned_memreg(sc_rm_mr_t mr, sc_faddr_t *addr_start, sc_faddr_t *addr_end)
{
	sc_err_t sciErr = 0;
	bool owned;
	sc_faddr_t start, end;

	owned = sc_rm_is_memreg_owned(-1, mr);
	if (owned) {
		sciErr = sc_rm_get_memreg_info(-1, mr, &start, &end);
		if (sciErr) {
			printf("Memreg get info failed, %d\n", sciErr);
			return -EINVAL;
		} else {
			debug("0x%llx -- 0x%llx\n", start, end);

			*addr_start = start;
			*addr_end = end;

			return 0;
		}
	}

	return -EINVAL;
}

phys_size_t get_effective_memsize(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end, start_aligned;
	int err;
	int ret;
	u64 dram_size;

	ret = get_dram_size(&dram_size);
	if(ret)
		return 0;
	u64 phys_sdram_1_size = dram_size;

	if (dram_size > 0x80000000ULL) {
		phys_sdram_1_size = 0x80000000ULL;
	}

	if (IS_ENABLED(CONFIG_XEN))
		return phys_sdram_1_size;

	for (mr = 0; mr < 64; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			start_aligned = roundup(start, MEMSTART_ALIGNMENT);
			if (start_aligned > end) /* Too small memory region, not use it */
				continue;

			/* Find the memory region runs the u-boot */
			if (start >= PHYS_SDRAM_1 && start <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size)
				&& (start <= CONFIG_TEXT_BASE && CONFIG_TEXT_BASE <= end)){
				if ((end + 1) <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size))
					return (end - PHYS_SDRAM_1 + 1);
				else
					return phys_sdram_1_size;
			}
		}
	}

	return phys_sdram_1_size;
}

static void dram_bank_sort(int current_bank)
{
	phys_addr_t start;
	phys_size_t size;
	while (current_bank > 0) {
		if (gd->bd->bi_dram[current_bank - 1].start > gd->bd->bi_dram[current_bank].start) {
			start = gd->bd->bi_dram[current_bank - 1].start;
			size = gd->bd->bi_dram[current_bank - 1].size;

			gd->bd->bi_dram[current_bank - 1].start = gd->bd->bi_dram[current_bank].start;
			gd->bd->bi_dram[current_bank - 1].size = gd->bd->bi_dram[current_bank].size;

			gd->bd->bi_dram[current_bank].start = start;
			gd->bd->bi_dram[current_bank].size = size;
		}

		current_bank--;
	}
}

int dram_init_banksize(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end;
	int i = 0;
	int err;

	uint64_t dram_size = 0;
	int ret = get_dram_size(&dram_size);
	if(ret)
		return 0;

	uint64_t phys_sdram_1_size = dram_size, phys_sdram_2_size = 0;

	if (dram_size > 0x80000000) {
		phys_sdram_1_size = 0x80000000;
		phys_sdram_2_size = dram_size - 0x80000000;
	}

	if (IS_ENABLED(CONFIG_XEN)) {
		gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[0].size = phys_sdram_1_size;
		gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
		gd->bd->bi_dram[1].size = phys_sdram_2_size;

		return 0;
	}

	for (mr = 0; mr < 64 && i < CONFIG_NR_DRAM_BANKS; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			start = roundup(start, MEMSTART_ALIGNMENT);
			if (start > end) /* Too small memory region, not use it */
				continue;

			if (start >= PHYS_SDRAM_1 && start <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size)) {
				gd->bd->bi_dram[i].start = start;

				if ((end + 1) <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size))
					gd->bd->bi_dram[i].size = end - start + 1;
				else
					gd->bd->bi_dram[i].size = ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size) - start;

				dram_bank_sort(i);
				i++;
			} else if (start >= PHYS_SDRAM_2 && start <= ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size)) {
				gd->bd->bi_dram[i].start = start;

				if ((end + 1) <= ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size))
					gd->bd->bi_dram[i].size = end - start + 1;
				else
					gd->bd->bi_dram[i].size = ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size) - start;

				dram_bank_sort(i);
				i++;
			}

		}
	}

	/* If error, set to the default value */
	if (!i) {
		gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[0].size = phys_sdram_1_size;
		gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
		gd->bd->bi_dram[1].size = phys_sdram_2_size;
	}

	return 0;
}

u64 get_dram_block_attrs(sc_faddr_t addr_start)
{
	u64 dram_size = 0;
	int ret = get_dram_size(&dram_size);
	if(ret)
		return 0;
	u64 phys_sdram_1_size = dram_size, phys_sdram_2_size = 0;

	if (dram_size > 0x80000000) {
		phys_sdram_1_size = 0x80000000;
		phys_sdram_2_size = dram_size - 0x80000000;
	}

	if ((addr_start >= PHYS_SDRAM_1 && addr_start <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size)) ||
	    (addr_start >= PHYS_SDRAM_2 && addr_start <= ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size)))
#ifdef CONFIG_IMX_TRUSTY_OS
		return (PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_INNER_SHARE);
#else
		return (PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_OUTER_SHARE);
#endif

	return (PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) | PTE_BLOCK_NON_SHARE | PTE_BLOCK_PXN | PTE_BLOCK_UXN);
}

u64 get_dram_block_size(sc_faddr_t addr_start, sc_faddr_t addr_end)
{
	u64 dram_size;
	int ret = get_dram_size(&dram_size);
	if(ret)
		return 0;
	u64 phys_sdram_1_size = dram_size, phys_sdram_2_size = 0;

	if (dram_size > 0x80000000) {
		phys_sdram_1_size = 0x80000000;
		phys_sdram_2_size = dram_size - 0x80000000;
	}

	if (addr_start >= PHYS_SDRAM_1 && addr_start <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size)) {
		if ((addr_end + 1) > ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size))
			return ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size) - addr_start;
	} else if (addr_start >= PHYS_SDRAM_2 && addr_start <= ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size)) {

		if ((addr_end + 1) > ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size))
			return ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size) - addr_start;
	}

	return (addr_end - addr_start + 1);
}

int dram_init(void)
{
	sc_rm_mr_t mr;
	sc_faddr_t start, end;
	int err;
	u64 dram_size;
	int ret = get_dram_size(&dram_size);
	if(ret)
		return 0;
	u64 phys_sdram_1_size = dram_size, phys_sdram_2_size = 0;

	if (dram_size > 0x80000000) {
		phys_sdram_1_size = 0x80000000;
		phys_sdram_2_size = dram_size - 0x80000000;
	}

	if (IS_ENABLED(CONFIG_XEN)) {
		gd->ram_size = phys_sdram_1_size;
		gd->ram_size += phys_sdram_1_size;

		return 0;
	}

	for (mr = 0; mr < 64; mr++) {
		err = get_owned_memreg(mr, &start, &end);
		if (!err) {
			start = roundup(start, MEMSTART_ALIGNMENT);
			if (start > end) /* Too small memory region, not use it */
				continue;

			if (start >= PHYS_SDRAM_1 && start <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size)) {

				if ((end + 1) <= ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size))
					gd->ram_size += end - start + 1;
				else
					gd->ram_size += ((sc_faddr_t)PHYS_SDRAM_1 + phys_sdram_1_size) - start;

			} else if (start >= PHYS_SDRAM_2 && start <= ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size)) {

				if ((end + 1) <= ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size))
					gd->ram_size += end - start + 1;
				else
					gd->ram_size += ((sc_faddr_t)PHYS_SDRAM_2 + phys_sdram_2_size) - start;
			}
		}
	}

	/* If error, set to the default value */
	if (!gd->ram_size) {
		gd->ram_size = phys_sdram_1_size;
		gd->ram_size += phys_sdram_2_size;
	}
	return 0;
}
#endif
