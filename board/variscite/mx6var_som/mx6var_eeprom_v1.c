/*
 * Copyright (C) 2016 Variscite Ltd.
 *
 * Setup DRAM parametes and calibration values
 * for the specific DRAM on the board.
 * The parameters were provided by
 * i.MX6 DDR Stress Test Tool and saved on EEPROM.
 *
 * For non-DART boards
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <i2c.h>
#include "mx6var_eeprom_v1.h"

static void var_eeprom_v1_print_production_info(const struct var_eeprom_v1_cfg *p_var_eeprom_v1_cfg)
{
	printf("\nPart number: %.*s\n",
			sizeof(p_var_eeprom_v1_cfg->header.part_number) - 1,
			(char *) p_var_eeprom_v1_cfg->header.part_number);

	printf("Assembly: %.*s\n",
			sizeof(p_var_eeprom_v1_cfg->header.assembly) - 1,
			(char *) p_var_eeprom_v1_cfg->header.assembly);

	printf("Date of production: %.*s\n",
			sizeof(p_var_eeprom_v1_cfg->header.date) - 1,
			(char *) p_var_eeprom_v1_cfg->header.date);
}

static int var_eeprom_write(uchar *ptr, u32 size, u32 eeprom_i2c_addr, u32 offset)
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
	while ((ret == 0) && (size_written < size_to_write)) {
		P0_select_page_EEPROM = (offset > 0xFF);
		chip = eeprom_i2c_addr + P0_select_page_EEPROM;
		addr = (offset & 0xFF);
		ret = i2c_write(chip, addr, 1, ptr, VAR_EEPROM_WRITE_MAX_SIZE);

		/* Wait for EEPROM write operation to complete (No ACK) */
		mdelay(11);

		size_written += VAR_EEPROM_WRITE_MAX_SIZE;
		offset += VAR_EEPROM_WRITE_MAX_SIZE;
		ptr += VAR_EEPROM_WRITE_MAX_SIZE;
	}

	return ret;
}

static int do_var_eeprom_params(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct var_eeprom_v1_cfg var_eeprom_v1_cfg;
	int offset;

	if (argc != 4)
		return -1;

	i2c_set_bus_num(VAR_EEPROM_I2C_BUS);
	if (i2c_probe(VAR_EEPROM_I2C_ADDR)) {
		printf("Error: Couldn't find EEPROM device\n");
		return -1;
	}

	memset(&var_eeprom_v1_cfg.header, 0, sizeof(var_eeprom_v1_cfg.header));

	strncpy((char *) var_eeprom_v1_cfg.header.part_number, argv[1], sizeof(var_eeprom_v1_cfg.header.part_number) - 1);
	strncpy((char *) var_eeprom_v1_cfg.header.assembly, argv[2], sizeof(var_eeprom_v1_cfg.header.assembly) - 1);
	strncpy((char *) var_eeprom_v1_cfg.header.date, argv[3], sizeof(var_eeprom_v1_cfg.header.date) - 1);

	var_eeprom_v1_print_production_info(&var_eeprom_v1_cfg);

	offset = (uchar *) var_eeprom_v1_cfg.header.part_number - (uchar *) &var_eeprom_v1_cfg;
	if (var_eeprom_write((uchar *) var_eeprom_v1_cfg.header.part_number,
				sizeof(var_eeprom_v1_cfg.header.part_number),
				VAR_EEPROM_I2C_ADDR,
				offset))
		goto err;

	offset = (uchar *) var_eeprom_v1_cfg.header.assembly - (uchar *) &var_eeprom_v1_cfg;
	if (var_eeprom_write((uchar *) var_eeprom_v1_cfg.header.assembly,
				sizeof(var_eeprom_v1_cfg.header.assembly),
				VAR_EEPROM_I2C_ADDR,
				offset))
		goto err;

	offset = (uchar *) var_eeprom_v1_cfg.header.date - (uchar *) &var_eeprom_v1_cfg;
	if (var_eeprom_write((uchar *) var_eeprom_v1_cfg.header.date,
				sizeof(var_eeprom_v1_cfg.header.date),
				VAR_EEPROM_I2C_ADDR,
				offset))
		goto err;

	printf("EEPROM updated successfully\n");

	return 0;

err:
	printf("Error writing to EEPROM!\n");
	return -1;
}

U_BOOT_CMD(
	vareeprom,	5,	1,	do_var_eeprom_params,
	"For internal use only",
	"- Do not use"
);

#ifdef CONFIG_SPL_BUILD
#include <asm/arch/mx6-ddr.h>
#include <asm/arch/sys_proto.h>

void var_set_ram_size(u32 ram_size);

static inline bool var_eeprom_v1_is_valid(const struct var_eeprom_v1_cfg *p_var_eeprom_v1_cfg)
{
	return (VARISCITE_MAGIC == p_var_eeprom_v1_cfg->header.variscite_magic);
}

static int var_eeprom_v1_read_struct(struct var_eeprom_v1_cfg *p_var_eeprom_v1_cfg)
{
	i2c_set_bus_num(VAR_EEPROM_I2C_BUS);
	if (i2c_probe(VAR_EEPROM_I2C_ADDR)) {
		eeprom_v1_debug("\nError: Couldn't find EEPROM device\n");
		return -1;
	}

	if (i2c_read(VAR_EEPROM_I2C_ADDR, 0, 1,
				(u8 *)p_var_eeprom_v1_cfg,
				sizeof(struct var_eeprom_v1_cfg))) {
		eeprom_v1_debug("\nError reading data from EEPROM\n");
		return -1;
	}

	if (!var_eeprom_v1_is_valid(p_var_eeprom_v1_cfg)) {
		eeprom_v1_debug("\nError: Data on EEPROM is invalid\n");
		return -1;
	}

	return 0;
}

static void var_eeprom_v1_mx6sdl_dram_setup_iomux(const struct var_pinmux_group_regs *pinmux_group)
{
	const struct mx6sdl_iomux_ddr_regs mx6_ddr_ioregs = {
		.dram_sdclk_0	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdclk_1	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_cas	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_ras	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_reset	= pinmux_group->dram_reset,
		.dram_sdcke0	= pinmux_group->dram_sdcke0,
		.dram_sdcke1	= pinmux_group->dram_sdcke1,
		.dram_sdba2	= pinmux_group->IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2,
		.dram_sdodt0	= pinmux_group->dram_sdodt0,
		.dram_sdodt1	= pinmux_group->dram_sdodt1,
		.dram_sdqs0	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs1	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs2	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs3	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs4	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs5	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs6	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs7	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm0	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm1	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm2	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm3	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm4	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm5	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm6	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm7	= pinmux_group->pinmux_ctrlpad_all_value,
	};

	const struct mx6sdl_iomux_grp_regs mx6_grp_ioregs = {
		.grp_ddr_type		= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE,
		.grp_ddrpke		= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRPKE,
		.grp_addds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_ctlds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_ddrmode_ctl	= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL,
		.grp_ddrmode		= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE,
		.grp_b0ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b1ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b2ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b3ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b4ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b5ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b6ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b7ds		= pinmux_group->pinmux_ctrlpad_all_value,
	};

	mx6sdl_dram_iocfg(64, &mx6_ddr_ioregs, &mx6_grp_ioregs);
}

static void var_eeprom_v1_mx6dq_dram_setup_iomux(const struct var_pinmux_group_regs *pinmux_group)
{
	const struct mx6dq_iomux_ddr_regs mx6_ddr_ioregs = {
		.dram_sdclk_0	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdclk_1	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_cas	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_ras	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_reset	= pinmux_group->dram_reset,
		.dram_sdcke0	= pinmux_group->dram_sdcke0,
		.dram_sdcke1	= pinmux_group->dram_sdcke1,
		.dram_sdba2	= pinmux_group->IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2,
		.dram_sdodt0	= pinmux_group->dram_sdodt0,
		.dram_sdodt1	= pinmux_group->dram_sdodt1,
		.dram_sdqs0	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs1	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs2	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs3	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs4	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs5	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs6	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_sdqs7	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm0	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm1	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm2	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm3	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm4	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm5	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm6	= pinmux_group->pinmux_ctrlpad_all_value,
		.dram_dqm7	= pinmux_group->pinmux_ctrlpad_all_value,
	};

	const struct mx6dq_iomux_grp_regs mx6_grp_ioregs = {
		.grp_ddr_type		= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE,
		.grp_ddrpke		= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRPKE,
		.grp_addds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_ctlds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_ddrmode_ctl	= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL,
		.grp_ddrmode		= pinmux_group->IOMUXC_SW_PAD_CTL_GRP_DDRMODE,
		.grp_b0ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b1ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b2ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b3ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b4ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b5ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b6ds		= pinmux_group->pinmux_ctrlpad_all_value,
		.grp_b7ds		= pinmux_group->pinmux_ctrlpad_all_value,
	};

	mx6dq_dram_iocfg(64, &mx6_ddr_ioregs, &mx6_grp_ioregs);
}

static void var_eeprom_v1_handle_write_opcodes(const struct reg_write_opcode write_opcodes[])
{
	volatile struct mmdc_p_regs *mmdc_p0 = (struct mmdc_p_regs *)MMDC_P0_BASE_ADDR;
	u32 opcode, address, value, i = 0;
	volatile u32 *reg_ptr;

	/* Go through all register initializations and apply to correct registers */
	while (i < VAR_EEPROM_MAX_NUM_OF_OPCODES) {
		address = write_opcodes[i].address;
		if (address == 0)
			break;
		reg_ptr = (u32 *)address;
		value = write_opcodes[i].value;

		opcode = address & 0xF0000000;
		switch (opcode) {
		case VAR_DDR_INIT_OPCODE_WRITE_VAL_TO_ADDRESS:
			/* ZQ calibration? ==> Need to wait? */
			if ((address == mmdc_p0->mpzqhwctrl) && (value & 0x3))
				while (mmdc_p0->mpzqhwctrl & 0x00010000);
			else
				/* write value to reg */
				*reg_ptr = value;
			break;
		case VAR_DDR_INIT_OPCODE_WAIT_MASK_RISING:
			while ((value & (*reg_ptr)) != value);
			break;
		case VAR_DDR_INIT_OPCODE_WAIT_MASK_FALLING:
			while ((value & ~(*reg_ptr)) != value);
			break;
		case VAR_DDR_INIT_OPCODE_DELAY_USEC:
			udelay(value);
			break;
		default:
			break;
		}

		i++;
	}

	udelay(500);
}

/*
 * Returns DRAM size in MiB
 */
static inline u32 var_eeprom_v1_get_ram_size(struct var_eeprom_v1_cfg *p_var_eeprom_v1_cfg)
{
	return p_var_eeprom_v1_cfg->header.dram_size;
}

int var_eeprom_v1_dram_init(void)
{
	struct var_eeprom_v1_cfg var_eeprom_v1_cfg = {{0}};

	if (var_eeprom_v1_read_struct(&var_eeprom_v1_cfg))
		return -1;

	if (is_mx6sdl())
		var_eeprom_v1_mx6sdl_dram_setup_iomux(&(var_eeprom_v1_cfg.pinmux_group));
	else if (is_mx6dq() || is_mx6dqp())
		var_eeprom_v1_mx6dq_dram_setup_iomux(&(var_eeprom_v1_cfg.pinmux_group));

	var_eeprom_v1_handle_write_opcodes(var_eeprom_v1_cfg.write_opcodes);

	var_set_ram_size(var_eeprom_v1_get_ram_size(&var_eeprom_v1_cfg));

	var_eeprom_v1_print_production_info(&var_eeprom_v1_cfg);

	return 0;
}
#endif /* CONFIG_SPL_BUILD */
