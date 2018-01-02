/*
 * Copyright (C) 2016 Variscite Ltd.
 *
 * Setup DRAM parametes and calibration values
 * for the specific DRAM on the board.
 * The parameters were provided by
 * i.MX6 DDR Stress Test Tool and saved on EEPROM.
 *
 * If data can't be read from the EEPROM, use default
 * "Legacy" values.
 *
 *
 * For DART board
 * (Can hold more parameters than "VAR EEPROM V1")
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef CONFIG_SPL_BUILD
#include <common.h>
#include <i2c.h>
#include "mx6var_eeprom_v2.h"
#include "mx6var_legacy_dart_auto.c"

void var_set_ram_size(u32 ram_size);


static inline bool var_eeprom_v2_is_valid(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	return (VARISCITE_MAGIC_V2 == p_var_eeprom_v2_cfg->variscite_magic);
}

static int var_eeprom_v2_read_struct(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	i2c_set_bus_num(VAR_DART_EEPROM_I2C_BUS);
	if (i2c_probe(VAR_DART_EEPROM_I2C_ADDR)) {
		eeprom_v2_debug("\nError: Couldn't find EEPROM device\n");
		return -1;
	}

	if (i2c_read(VAR_DART_EEPROM_I2C_ADDR, 0, 1,
				(u8 *) p_var_eeprom_v2_cfg,
				sizeof(struct var_eeprom_v2_cfg))) {
		eeprom_v2_debug("\nError reading data from EEPROM\n");
		return -1;
	}

	if (!var_eeprom_v2_is_valid(p_var_eeprom_v2_cfg)) {
		eeprom_v2_debug("\nError: Data on EEPROM is invalid\n");
		return -1;
	}

	return 0;
}

/*
 * Fills custom_addresses & custom_values, from custom_addresses_values
 */
static void load_custom_data(u32 *custom_addresses, u32 *custom_values, const u32 *custom_addresses_values)
{
	int i, j=0;

	for (i = 0; i < MAX_CUSTOM_ADDRESSES; i++) {
		if (custom_addresses_values[i] == 0)
			break;
		custom_addresses[i] = custom_addresses_values[i];
	}

	i++;
	if (i > MAX_CUSTOM_ADDRESSES)
		return;

	j=0;
	for (; i < MAX_CUSTOM_VALUES; i++) {
		if (custom_addresses_values[i] == 0)
			break;
		custom_values[j] = custom_addresses_values[i];
		j++;
	}
}

static u32 get_address_by_index(u8 index, const u32 *common_addresses, const u32 *custom_addresses)
{
	if (index >= MAX_COMMON_ADDRS_INDEX)
		return custom_addresses[index - MAX_COMMON_ADDRS_INDEX];

	return common_addresses[index];
}

static u32 get_value_by_index(u8 index, const u32 *common_values, const u32 *custom_values)
{
	if (index >= MAX_COMMON_VALUES_INDEX)
		return custom_values[index - MAX_COMMON_VALUES_INDEX];

	return common_values[index];
}

static int handle_commands(const struct eeprom_command commands[],
			const u32 *common_addresses,
			const u32 *common_values,
			const u32 *custom_addresses,
			const u32 *custom_values)
{
	u32 address, value;
	volatile u32 *reg_ptr;
	u8 wait_idx = 0;
	int i = 0;

	while (i < MAX_NUM_OF_COMMANDS) {

		eeprom_v2_debug("Executing command %03d: %03d,  %03d\n",
				i,
				commands[i].address_index,
				commands[i].value_index);

		if (commands[i].address_index == LAST_COMMAND_INDEX)
			return 0;

		if (commands[i].address_index == DELAY_10USEC_INDEX) {
			/* Delay for Value * 10 uSeconds */
			eeprom_v2_debug("Delaying for %d microseconds\n", commands[i].value_index * 10);
			udelay((int)(commands[i].value_index * 10));
			++i;
			continue;
		}

		/*
		 * Check for a wait index.
		 * A wait index means "next command is a wait command".
		 */
		switch (commands[i].address_index) {
		case WHILE_NOT_EQUAL_INDEX:
		case WHILE_EQUAL_INDEX:
		case WHILE_AND_INDEX:
		case WHILE_NOT_AND_INDEX:
			/* Save wait index and go to next command */
			wait_idx = commands[i].address_index;
			++i;
			break;
		}

		/* Get address and value */
		address = get_address_by_index(commands[i].address_index, common_addresses, custom_addresses);
		value = get_value_by_index(commands[i].value_index, common_values, custom_values);
		reg_ptr = (u32 *)address;

		switch (wait_idx) {
		case WHILE_NOT_EQUAL_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x is not equal %08x\n", address, value);
			while(*reg_ptr != value);
			break;
		case WHILE_EQUAL_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x is equal %08x\n", address, value);
			while(*reg_ptr == value);
			break;
		case WHILE_AND_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x and %08x is not zero\n", address, value);
			while(*reg_ptr & value);
			break;
		case WHILE_NOT_AND_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x and %08x is zero\n", address, value);
			while(!(*reg_ptr & value));
			break;
		default:
			/* This is a regular set command (non-wait) */
			eeprom_v2_debug("Setting data at address %08x to %08x\n", address, value);
			*reg_ptr = value;
			break;
		}

		++i;
	}

	return 0;
}

/*
 * Returns DRAM size in MiB
 */
static inline u32 var_eeprom_v2_get_ram_size(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	return p_var_eeprom_v2_cfg->dram_size * 128;
}

static void var_eeprom_v2_print_production_info(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	printf("\nPart number: %.*s\n",
			sizeof(p_var_eeprom_v2_cfg->part_number) - 1,
			(char *) p_var_eeprom_v2_cfg->part_number);

	printf("Assembly: %.*s\n",
			sizeof(p_var_eeprom_v2_cfg->assembly) - 1,
			(char *) p_var_eeprom_v2_cfg->assembly);

	printf("Date of production: %.*s\n",
			sizeof(p_var_eeprom_v2_cfg->date) - 1,
			(char *) p_var_eeprom_v2_cfg->date);
}

void var_eeprom_v2_dram_init(void)
{
	struct var_eeprom_v2_cfg var_eeprom_v2_cfg = {0};
	int is_eeprom_valid = !(var_eeprom_v2_read_struct(&var_eeprom_v2_cfg));
	int is_eeprom_data_correct = is_eeprom_valid;

	/*
	 * The eeprom contains commands with
	 * 1 byte index to a common address in this array, and
	 * 1 byte index to a common value in the next array - to write to the address.
	 */
	const u32 common_addresses[]=
	{
		#include "addresses.inc"
	};

	const u32 common_values[] =
	{
		#include "values.inc"
	};

	/*
	 * Some commands in the eeprom contain higher indices,
	 * to custom addresses and values which are not present in the common arrays,
	 * and it also contains an array of the custom addresses and values themselves.
	 */
	u32 custom_addresses[MAX_CUSTOM_ADDRESSES]={0};
	u32 custom_values[MAX_CUSTOM_VALUES]={0};

	/*
	 * The MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_commands
	 * revision is incorrect.
	 * If the data is equal to it, use mt128x64mx32_Step3_commands revision instead
	 */
	if (is_eeprom_valid &&
			!memcmp(var_eeprom_v2_cfg.commands,
				MX6Q_MMDC_LPDDR2_register_programming_aid_v0_Micron_InterL_commands,
				254))
			is_eeprom_data_correct=0;

	load_custom_data(custom_addresses, custom_values,
			is_eeprom_data_correct ?
			var_eeprom_v2_cfg.custom_addresses_values:
			mt128x64mx32_Step3_RamValues);

	handle_commands(is_eeprom_data_correct ?
			var_eeprom_v2_cfg.commands :
			(struct eeprom_command *) mt128x64mx32_Step3_commands,
			common_addresses, common_values, custom_addresses, custom_values);


	if (is_eeprom_valid) {
		var_set_ram_size(var_eeprom_v2_get_ram_size(&var_eeprom_v2_cfg));
		var_eeprom_v2_print_production_info(&var_eeprom_v2_cfg);
	} else {
		var_set_ram_size(1024);
		printf("\nDDR LEGACY configuration\n");
	}
}
#endif /* CONFIG_SPL_BUILD */
