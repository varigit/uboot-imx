/*
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <i2c.h>
#include "mx6var_eeprom.h"
#ifdef CONFIG_SPL_BUILD
#include <asm/arch/mx6-ddr.h>

#ifdef EEPROM_DEBUG
#define eeprom_debug(M, ...) printf("EEPROM DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_debug(M, ...)
#endif

bool var_eeprom_is_valid(struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	return (VARISCITE_MAGIC == p_var_eeprom_cfg->header.variscite_magic);
}

void var_eeprom_mx6dlsl_dram_setup_iomux_from_struct(struct var_pinmux_group_regs *pinmux_group)
{
	volatile struct mx6sdl_iomux_ddr_regs *mx6dl_ddr_iomux;
	volatile struct mx6sdl_iomux_grp_regs *mx6dl_grp_iomux;

	mx6dl_ddr_iomux = (struct mx6sdl_iomux_ddr_regs *) MX6SDL_IOM_DDR_BASE;
	mx6dl_grp_iomux = (struct mx6sdl_iomux_grp_regs *) MX6SDL_IOM_GRP_BASE;

	mx6dl_grp_iomux->grp_ddr_type 		= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE;
	mx6dl_grp_iomux->grp_ddrpke 		= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRPKE;
	mx6dl_ddr_iomux->dram_sdclk_0 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdclk_1 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_cas 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_ras 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_addds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_reset 		= (u32)pinmux_group->dram_reset;
	mx6dl_ddr_iomux->dram_sdcke0 		= (u32)pinmux_group->dram_sdcke0;
	mx6dl_ddr_iomux->dram_sdcke1 		= (u32)pinmux_group->dram_sdcke1;
	mx6dl_ddr_iomux->dram_sdba2 		= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2;
	mx6dl_ddr_iomux->dram_sdodt0 		= (u32)pinmux_group->dram_sdodt0;
	mx6dl_ddr_iomux->dram_sdodt1 		= (u32)pinmux_group->dram_sdodt1;
	mx6dl_grp_iomux->grp_ctlds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_ddrmode_ctl	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL;
	mx6dl_ddr_iomux->dram_sdqs0 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs1 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs2 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs3 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs4 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs5 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs6 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs7 	 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_ddrmode	 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE;
	mx6dl_grp_iomux->grp_b0ds		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b1ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b2ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b3ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b4ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b5ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b6ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b7ds 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm0 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm1 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm2 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm3 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm4 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm5 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm6 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm7 		= (u32)pinmux_group->pinmux_ctrlpad_all_value;
}

void var_eeprom_mx6qd_dram_setup_iomux_from_struct(struct var_pinmux_group_regs *pinmux_group)
{
	volatile struct mx6dq_iomux_ddr_regs *mx6q_ddr_iomux;
	volatile struct mx6dq_iomux_grp_regs *mx6q_grp_iomux;

	mx6q_ddr_iomux = (struct mx6dq_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;
	mx6q_grp_iomux = (struct mx6dq_iomux_grp_regs *) MX6DQ_IOM_GRP_BASE;

	mx6q_grp_iomux->grp_ddr_type 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE;
	mx6q_grp_iomux->grp_ddrpke	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRPKE;
	mx6q_ddr_iomux->dram_sdclk_0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdclk_1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_cas 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_ras 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_addds	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_reset	= (u32)pinmux_group->dram_reset;
	mx6q_ddr_iomux->dram_sdcke0 	= (u32)pinmux_group->dram_sdcke0;
	mx6q_ddr_iomux->dram_sdcke1 	= (u32)pinmux_group->dram_sdcke1;
	mx6q_ddr_iomux->dram_sdba2 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2;
	mx6q_ddr_iomux->dram_sdodt0 	= (u32)pinmux_group->dram_sdodt0;
	mx6q_ddr_iomux->dram_sdodt1 	= (u32)pinmux_group->dram_sdodt1;
	mx6q_grp_iomux->grp_ctlds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_ddrmode_ctl = (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL;
	mx6q_ddr_iomux->dram_sdqs0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs2 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs3 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs4 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs5 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs6 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdqs7 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_ddrmode 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE;
	mx6q_grp_iomux->grp_b0ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b1ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b2ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b3ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b4ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b5ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b6ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_b7ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm2 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm3 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm4 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm5 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm6 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_dqm7 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
}

void var_eeprom_dram_init_from_struct(struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	volatile struct mmdc_p_regs *mmdc_p0 = (struct mmdc_p_regs *) MMDC_P0_BASE_ADDR;
	u32 i;
	u32 last;
	u32 opcode;

	/* Go through all register initializations and apply to correct registers... */
	i = 0;
	last = sizeof(p_var_eeprom_cfg->write_opcodes) / sizeof(u32);
	while ( (0 != p_var_eeprom_cfg->write_opcodes[i].address) && (i < last) ) {
		opcode = p_var_eeprom_cfg->write_opcodes[i].address & 0xF0000000;
		switch (opcode) {
		case VAR_DDR_INIT_OPCODE_WRITE_VAL_TO_ADDRESS:
				/* ZQ calibration? ==> Need to wait? */
				if (mmdc_p0->mpzqhwctrl == p_var_eeprom_cfg->write_opcodes[i].address) {
					if (p_var_eeprom_cfg->write_opcodes[i].value & 0x3) {
						while (mmdc_p0->mpzqhwctrl & 0x00010000);
					}
				}
				else {
					/* write value to reg */
					volatile u32 *reg_ptr = (volatile u32 *)p_var_eeprom_cfg->write_opcodes[i].address;

					*reg_ptr = p_var_eeprom_cfg->write_opcodes[i].value;
				}
				break;
		case VAR_DDR_INIT_OPCODE_WAIT_MASK_RISING: {
				volatile u32 *reg_ptr = (volatile u32 *)p_var_eeprom_cfg->write_opcodes[i].address;

				while ( (p_var_eeprom_cfg->write_opcodes[i].value & *reg_ptr) != \
						p_var_eeprom_cfg->write_opcodes[i].value );
				break;
			}
		case VAR_DDR_INIT_OPCODE_WAIT_MASK_FALLING: {
				volatile u32 *reg_ptr = (volatile u32 *)p_var_eeprom_cfg->write_opcodes[i].address;

				while ( (p_var_eeprom_cfg->write_opcodes[i].value & (~(*reg_ptr)) ) != \
						p_var_eeprom_cfg->write_opcodes[i].value );
				break;
			}
		case VAR_DDR_INIT_OPCODE_DELAY_USEC:
			udelay(p_var_eeprom_cfg->write_opcodes[i].value);
			break;
		default:
			break;
		}

		i++;
	}

	/* Short delay */
	udelay(500);
}

#ifdef EEPROM_DEBUG
static void var_eeprom_printf_array_dwords(u32 *ptr, u32 address_mem, u32 size)
{
	u32 idx;
	u32 *p_end = ptr + (size/4);
	idx = 0;
	while (ptr < p_end) {
		if ((idx & 0x3) == 0) {
			printf("\n0x%08x:", address_mem);
			address_mem += 0x10;
		}
		printf(" 0x%08x", *ptr);
		idx++;
		ptr++;
	}
	printf("\n");
}
#endif

int var_eeprom_read_struct(struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	int eeprom_found;
	int ret = 0;
	i2c_set_bus_num(1);
	eeprom_found = i2c_probe(VAR_MX6_EEPROM_CHIP);
	eeprom_debug("eeprom_found(0x%x)=%d\n", VAR_MX6_EEPROM_CHIP, eeprom_found);
	if (0 == eeprom_found) {
		eeprom_debug("EEPROM device detected, address=0x%x\n", VAR_MX6_EEPROM_CHIP);

		if (i2c_read(VAR_MX6_EEPROM_CHIP, VAR_MX6_EEPROM_STRUCT_OFFSET, \
					1, (uchar *)p_var_eeprom_cfg, sizeof(struct var_eeprom_cfg))) {
			eeprom_debug("Read device ID error!\n");
			return -1;
		} else {
			/* Success */
#ifdef EEPROM_DEBUG
			var_eeprom_printf_array_dwords((u32 *) p_var_eeprom_cfg, (u32) 0, \
					sizeof(struct var_eeprom_cfg));
#endif
		}
	} else {
		eeprom_debug("Error! Couldn't find EEPROM device\n");
	}

	return ret;
}
#endif /* CONFIG_SPL_BUILD */

void var_eeprom_strings_print(struct var_eeprom_cfg *p_var_eeprom_cfg)
{
	p_var_eeprom_cfg->header.part_number[sizeof(p_var_eeprom_cfg->header.part_number)-1] = (u8)0x00;
	p_var_eeprom_cfg->header.Assembly[sizeof(p_var_eeprom_cfg->header.Assembly)-1] = (u8)0x00;
	p_var_eeprom_cfg->header.date[sizeof(p_var_eeprom_cfg->header.date)-1] = (u8)0x00;

	printf("Part number: %s\n", (char *)p_var_eeprom_cfg->header.part_number);
	printf("Assembly: %s\n", (char *)p_var_eeprom_cfg->header.Assembly);
	printf("Date of production: %s\n", (char *)p_var_eeprom_cfg->header.date);
}

static int var_eeprom_write(uchar *ptr, u32 size, u32 offset)
{
	int ret = 0;
	u32 size_written;
	u32 size_to_write;
	u32 P0_select_page_EEPROM;
	u32 chip;
	u32 addr;

	/* Write to EEPROM device */
	size_written = 0;
	size_to_write = size;
	while ((0 == ret) && (size_written < size_to_write)) {
		P0_select_page_EEPROM = (offset > 0xFF);
		chip = VAR_MX6_EEPROM_CHIP + P0_select_page_EEPROM;
		addr = (offset & 0xFF);
		ret = i2c_write(chip, addr, 1, ptr, VAR_MX6_EEPROM_WRITE_MAX_SIZE);

		/* Wait for EEPROM write operation to complete (No ACK) */
		mdelay(11);

		size_written += VAR_MX6_EEPROM_WRITE_MAX_SIZE;
		offset += VAR_MX6_EEPROM_WRITE_MAX_SIZE;
		ptr += VAR_MX6_EEPROM_WRITE_MAX_SIZE;
	}

	return ret;
}

/*
 * vareeprom command intepreter.
 */
static int do_var_eeprom_params(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct var_eeprom_cfg var_eeprom_cfg;
	int offset;

	if (argc != 4)
		return -1;

	memset(&var_eeprom_cfg, 0x00, sizeof(var_eeprom_cfg));

	memcpy(&var_eeprom_cfg.header.part_number[0], argv[1], sizeof(var_eeprom_cfg.header.part_number));
	memcpy(&var_eeprom_cfg.header.Assembly[0], argv[2], sizeof(var_eeprom_cfg.header.Assembly));
	memcpy(&var_eeprom_cfg.header.date[0], argv[3], sizeof(var_eeprom_cfg.header.date));

	var_eeprom_strings_print(&var_eeprom_cfg);

	offset = (uchar *)&var_eeprom_cfg.header.part_number[0] - (uchar *)&var_eeprom_cfg.header;
	if (var_eeprom_write((uchar *)&var_eeprom_cfg.header.part_number[0], \
				sizeof(var_eeprom_cfg.header.part_number), \
				VAR_MX6_EEPROM_STRUCT_OFFSET + offset) ) {
		printf("Error writing to EEPROM!\n");
		return -1;
	}

	offset = (uchar *)&var_eeprom_cfg.header.Assembly[0] - (uchar *)&var_eeprom_cfg;
	if (var_eeprom_write((uchar *)&var_eeprom_cfg.header.Assembly[0], \
				sizeof(var_eeprom_cfg.header.Assembly), \
				VAR_MX6_EEPROM_STRUCT_OFFSET + offset) ) {
		printf("Error writing to EEPROM!\n");
		return -1;
	}

	offset = (uchar *)&var_eeprom_cfg.header.date[0] - (uchar *)&var_eeprom_cfg;
	if (var_eeprom_write((uchar *)&var_eeprom_cfg.header.date[0], \
				sizeof(var_eeprom_cfg.header.date), \
				VAR_MX6_EEPROM_STRUCT_OFFSET + offset) ) {
		printf("Error writing to EEPROM!\n");
		return -1;
	}

	printf("EEPROM updated successfully\n");

	return 0;
}

U_BOOT_CMD(
	vareeprom,	5,	1,	do_var_eeprom_params,
	"For internal use only",
	"- Do not use"
);
