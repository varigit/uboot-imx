/*
 * Copyright (C) 2014 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite VAR_SOM_MX6 board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#ifdef CONFIG_SPL
#include <spl.h>
#include <i2c.h>
#endif

#ifdef CONFIG_I2C_MXC
void setup_local_i2c(void);
#endif

#include <asm/arch/mx6_ddr_regs.h>
#include "mx6var_eeprom.h"

/* #define EEPROM_DEBUG */
bool var_eeprom_is_valid(var_eeprom_config_struct_t *p_var_eeprom_cfg)
{
	bool valid = false;

	if (VARISCITE_MAGIC == p_var_eeprom_cfg->header.variscite_magic)
	{
		valid = true;
	}

	return valid;
}


void var_eeprom_mx6dlsl_dram_setup_iomux_from_struct(var_pinmux_group_regs_t *pinmux_group)
{
	volatile struct mx6sdl_iomux_ddr_regs *mx6dl_ddr_iomux;
	volatile struct mx6sdl_iomux_grp_regs *mx6dl_grp_iomux;

	mx6dl_ddr_iomux = (struct mx6sdl_iomux_ddr_regs *) MX6SDL_IOM_DDR_BASE;
	mx6dl_grp_iomux = (struct mx6sdl_iomux_grp_regs *) MX6SDL_IOM_GRP_BASE;

	mx6dl_grp_iomux->grp_ddr_type 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE;
	mx6dl_grp_iomux->grp_ddrpke 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRPKE;
	mx6dl_ddr_iomux->dram_sdclk_0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdclk_1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_cas 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_ras 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_addds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_reset 	= (u32)pinmux_group->dram_reset;
	mx6dl_ddr_iomux->dram_sdcke0 	= (u32)pinmux_group->dram_sdcke0;
	mx6dl_ddr_iomux->dram_sdcke1 	= (u32)pinmux_group->dram_sdcke1;
	mx6dl_ddr_iomux->dram_sdba2 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2;
	mx6dl_ddr_iomux->dram_sdodt0 	= (u32)pinmux_group->dram_sdodt0;
	mx6dl_ddr_iomux->dram_sdodt1 	= (u32)pinmux_group->dram_sdodt1;
	mx6dl_grp_iomux->grp_ctlds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_ddrmode_ctl= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL;
	mx6dl_ddr_iomux->dram_sdqs0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs2 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs3 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs4 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs5 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs6 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_sdqs7 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_ddrmode 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE;
	mx6dl_grp_iomux->grp_b0ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b1ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b2ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b3ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b4ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b5ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b6ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_grp_iomux->grp_b7ds 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm2 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm3 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm4 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm5 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm6 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6dl_ddr_iomux->dram_dqm7 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
}


void var_eeprom_mx6qd_dram_setup_iomux_from_struct(var_pinmux_group_regs_t *pinmux_group)
{
	volatile struct mx6qd_iomux_ddr_regs *mx6q_ddr_iomux;
	volatile struct mx6qd_iomux_grp_regs *mx6q_grp_iomux;

	mx6q_ddr_iomux = (struct mx6dqd_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;
	mx6q_grp_iomux = (struct mx6dqd_iomux_grp_regs *) MX6DQ_IOM_GRP_BASE;

	mx6q_grp_iomux->grp_ddr_type 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE;
	mx6q_grp_iomux->grp_ddrpke 	= (u32)pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRPKE;
	mx6q_ddr_iomux->dram_sdclk_0 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_sdclk_1 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_cas 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_ras 	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_grp_iomux->grp_addds	= (u32)pinmux_group->pinmux_ctrlpad_all_value;
	mx6q_ddr_iomux->dram_reset 	= (u32)pinmux_group->dram_reset;
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


void var_eeprom_dram_init_from_struct(var_eeprom_config_struct_t *p_var_eeprom_cfg)
{
	volatile struct mmdc_p_regs *mmdc_p0;
	volatile struct mmdc_p_regs *mmdc_p1;
	u32 i;
	u32 last;
	u32 opcode;

	/* Go through all register initializations and apply to correct registers... */
	i = 0;
	last = sizeof(p_var_eeprom_cfg->write_opcodes) / sizeof(u32);
	while ( (0 != p_var_eeprom_cfg->write_opcodes[i].address) && (i < last) )
	{
		opcode = p_var_eeprom_cfg->write_opcodes[i].address & 0xF0000000;
		switch (opcode)
		{
		case VAR_DDR_INIT_OPCODE_WRITE_VAL_TO_ADDRESS:
		{
			/* ZQ calibration? ==> Need to wait? */
			if (mmdc_p0->mpzqhwctrl == p_var_eeprom_cfg->write_opcodes[i].address)
			{
				if (p_var_eeprom_cfg->write_opcodes[i].value & 0x3)
				{
					while (mmdc_p0->mpzqhwctrl & 0x00010000)
						;
				}
			}
			else
			{
				/* write value to reg */
				volatile u32 *reg_ptr = (volatile u32 *)p_var_eeprom_cfg->write_opcodes[i].address;

				*reg_ptr = p_var_eeprom_cfg->write_opcodes[i].value;
			}

			break;
		}

		case VAR_DDR_INIT_OPCODE_WAIT_MASK_RISING:
		{
			volatile u32 *reg_ptr = (volatile u32 *)p_var_eeprom_cfg->write_opcodes[i].address;

			while ( (p_var_eeprom_cfg->write_opcodes[i].value & *reg_ptr) != p_var_eeprom_cfg->write_opcodes[i].value )
				;

			break;
		}

		case VAR_DDR_INIT_OPCODE_WAIT_MASK_FALLING:
		{
			volatile u32 *reg_ptr = (volatile u32 *)p_var_eeprom_cfg->write_opcodes[i].address;

			while ( (p_var_eeprom_cfg->write_opcodes[i].value & (~(*reg_ptr)) ) != p_var_eeprom_cfg->write_opcodes[i].value ) 
				;

			break;
		}

		case VAR_DDR_INIT_OPCODE_DELAY_USEC:
		{
			udelay(p_var_eeprom_cfg->write_opcodes[i].value);
			break;
		}

		default:
			break;
		}

		i++;
	}

	/* Short delay */
	udelay(500);
}


int var_eeprom_i2c_init(void)
{
	int ret = 0;

#ifdef CONFIG_I2C_MXC
	setup_local_i2c();
#endif

#ifdef CONFIG_SYS_I2C
	i2c_init_all();
#else
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif

	/* Select I2C bus number */
	i2c_set_bus_num(CONFIG_VARISCITE_I2C_SYS_BUS_NUM);

	return ret;
}


int var_eeprom_read_struct(var_eeprom_config_struct_t *p_var_eeprom_cfg)
{
	int eeprom_found;
	int ret = 0;

	eeprom_found = i2c_probe(VARISCITE_MX6_EEPROM_CHIP);
#ifdef EEPROM_DEBUG
	printf("eeprom_found(0x%x)=%d\n", VARISCITE_MX6_EEPROM_CHIP, eeprom_found);
#endif	
	if (0 == eeprom_found)
	{
#ifdef EEPROM_DEBUG
		printf("EEPROM device detected, address=0x%02x\n", VARISCITE_MX6_EEPROM_CHIP);
#endif		
		if (i2c_read(VARISCITE_MX6_EEPROM_CHIP, 
				VARISCITE_MX6_EEPROM_STRUCT_OFFSET, 
				1, 
				(uchar *)p_var_eeprom_cfg, 
				sizeof(var_eeprom_config_struct_t)))
		{
#ifdef EEPROM_DEBUG
			printf("Read device ID error!\n");
#endif
			return -1;
		} else {
			/* Success */
#ifdef EEPROM_DEBUG
			var_eeprom_printf_array_dwords((u32 *)p_var_eeprom_cfg, (u32)0, sizeof(var_eeprom_config_struct_t));
#endif
		}
	} else {
#ifdef EEPROM_DEBUG
		printf("Error! Couldn't find EEPROM device\n");
#endif
	}

	return ret;
}

void var_eeprom_strings_print(var_eeprom_config_struct_t *p_var_eeprom_cfg)
{
	p_var_eeprom_cfg->header.part_number[sizeof(p_var_eeprom_cfg->header.part_number)-1] = (u8)0x00;
	p_var_eeprom_cfg->header.Assembly[sizeof(p_var_eeprom_cfg->header.Assembly)-1] = (u8)0x00;
	p_var_eeprom_cfg->header.date[sizeof(p_var_eeprom_cfg->header.date)-1] = (u8)0x00;

	printf("Part number: %s\n", (char *)p_var_eeprom_cfg->header.part_number);
	printf("Assembly: %s\n", (char *)p_var_eeprom_cfg->header.Assembly);
	printf("Date of production: %s\n", (char *)p_var_eeprom_cfg->header.date);
}

#ifdef EEPROM_DEBUG
void var_eeprom_printf_array_dwords(u32 *ptr, u32 address_mem, u32 size)
{
	u32 idx;
	u32 *p_end = ptr + (size/4);

	idx = 0;
	while (ptr < p_end)
	{
		if ((idx & 0x3) == 0)
		{
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



int var_eeprom_write(uchar *ptr, u32 size, u32 offset)
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
	while ((0 == ret) && (size_written < size_to_write))
	{
		P0_select_page_EEPROM = (offset > 0xFF);
		chip = VARISCITE_MX6_EEPROM_CHIP + P0_select_page_EEPROM;
		addr = (offset & 0xFF);
		ret = i2c_write(chip,
				addr,
				1,
				ptr,
				VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE);

		/* Wait for EEPROM write operation to complete (No ACK) */
		udelay(11000);

		size_written += VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE;
		offset += VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE;
		ptr += VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE;
	}

	return ret;
}


/******************************************************************************
 * var_eeprom_params command intepreter.
 */
static int do_var_eeprom_params(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	var_eeprom_config_struct_t var_eeprom_cfg;
	int offset;

	if (argc != 4)
	{
		return -1;
	}

	memset(&var_eeprom_cfg, 0x00, sizeof(var_eeprom_cfg));

	memcpy(&var_eeprom_cfg.header.part_number[0], argv[1], sizeof(var_eeprom_cfg.header.part_number));
	memcpy(&var_eeprom_cfg.header.Assembly[0], argv[2], sizeof(var_eeprom_cfg.header.Assembly));
	memcpy(&var_eeprom_cfg.header.date[0], argv[3], sizeof(var_eeprom_cfg.header.date));

	var_eeprom_cfg.header.part_number[sizeof(var_eeprom_cfg.header.part_number)-1] = (u8)0x00;
	var_eeprom_cfg.header.Assembly[sizeof(var_eeprom_cfg.header.Assembly)-1] = (u8)0x00;
	var_eeprom_cfg.header.date[sizeof(var_eeprom_cfg.header.date)-1] = (u8)0x00;

	printf("Part number: %s\n", (char *)var_eeprom_cfg.header.part_number);
	printf("Assembly: %s\n", (char *)var_eeprom_cfg.header.Assembly);
	printf("Date of production: %s\n", (char *)var_eeprom_cfg.header.date);

	offset = (uchar *)&var_eeprom_cfg.header.part_number[0] - (uchar *)&var_eeprom_cfg.header;
	if (var_eeprom_write((uchar *)&var_eeprom_cfg.header.part_number[0],
				sizeof(var_eeprom_cfg.header.part_number),
				VARISCITE_MX6_EEPROM_STRUCT_OFFSET + offset) )
	{
		printf("Error writing to EEPROM!\n");
		return -1;
	} 

	offset = (uchar *)&var_eeprom_cfg.header.Assembly[0] - (uchar *)&var_eeprom_cfg;
	if (var_eeprom_write((uchar *)&var_eeprom_cfg.header.Assembly[0],
				sizeof(var_eeprom_cfg.header.Assembly),
				VARISCITE_MX6_EEPROM_STRUCT_OFFSET + offset) )
	{
		printf("Error writing to EEPROM!\n");
		return -1;
	} 

	offset = (uchar *)&var_eeprom_cfg.header.date[0] - (uchar *)&var_eeprom_cfg;
	if (var_eeprom_write((uchar *)&var_eeprom_cfg.header.date[0],
				sizeof(var_eeprom_cfg.header.date),
				VARISCITE_MX6_EEPROM_STRUCT_OFFSET + offset) )
	{
		printf("Error writing to EEPROM!\n");
		return -1;
	}

	printf("EEPROM updated successfully\n");

	return 0;
}

U_BOOT_CMD(
	vareeprom,	5,	1,	do_var_eeprom_params,
	"",
	""
);
