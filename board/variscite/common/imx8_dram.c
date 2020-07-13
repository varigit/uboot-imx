#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/arch-imx8/sci/sci.h>
#include "imx8_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_IMX8_BOARD_INIT_DRAM

/* Return DRAM size in bytes */
static u64 get_dram_size(void)
{
	int ret;
	u32 dram_size_mb;
	struct var_eeprom eeprom;

	ret = var_eeprom_read_header(&eeprom);
	if (ret)
		return (1ULL * DEFAULT_DRAM_SIZE_MB ) << 20;

	var_eeprom_get_dram_size(&eeprom, &dram_size_mb);

	return (1ULL * dram_size_mb) << 20;
}

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

	u64 dram_size = get_dram_size();
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
				&& (start <= CONFIG_SYS_TEXT_BASE && CONFIG_SYS_TEXT_BASE <= end)){
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
	uint64_t dram_size = get_dram_size();
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
	u64 dram_size = get_dram_size();
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
	u64 dram_size = get_dram_size();
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
	u64 dram_size = get_dram_size();
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
