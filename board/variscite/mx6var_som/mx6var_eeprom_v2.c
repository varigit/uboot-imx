/*
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifdef CONFIG_SPL_BUILD
#include <common.h>
#include <i2c.h>
#include "mx6var_eeprom_v2.h"

#ifdef EEPROM_V2_DEBUG
#define eeprom_v2_debug(M, ...) printf("EEPROM_V2 DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_v2_debug(M, ...)
#endif

static u32 get_address_by_index(unsigned char index, u32 *ram_addresses);
static u32 get_value_by_index(unsigned char index, u32 *ram_values);
static int handle_one_command(struct eeprom_command *eeprom_commands,int command_num, \
		u32 *ram_addresses, u32 *ram_values);


bool var_eeprom_v2_is_valid(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	return (VARISCITE_MAGIC_V2 == p_var_eeprom_v2_cfg->variscite_magic);
}


void var_eeprom_v2_strings_print(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	p_var_eeprom_v2_cfg->part_number[sizeof(p_var_eeprom_v2_cfg->part_number)-1] = (u8)0x00;
	p_var_eeprom_v2_cfg->Assembly[sizeof(p_var_eeprom_v2_cfg->Assembly)-1] = (u8)0x00;
	p_var_eeprom_v2_cfg->date[sizeof(p_var_eeprom_v2_cfg->date)-1] = (u8)0x00;

	printf("Part number: %s\n", (char *)p_var_eeprom_v2_cfg->part_number);
	printf("Assembly: %s\n", (char *)p_var_eeprom_v2_cfg->Assembly);
	printf("Date of production: %s\n", (char *)p_var_eeprom_v2_cfg->date);
}


void load_custom_data(u32 *custom_addresses_values, u32 *ram_addresses, u32 *ram_values)
{
	int i, j=0;

	for (i=0; i<MAXIMUM_RAM_ADDRESSES; i++) {
		ram_addresses[i]=0;
	}
	for (i=0; i<MAXIMUM_RAM_VALUES; i++) {
		ram_values[i]=0;
	}

	for (i=0; i<MAXIMUM_RAM_ADDRESSES; i++) {
		if (custom_addresses_values[i]==0)
			break;
		ram_addresses[j]=custom_addresses_values[i];
		j++;
	}

	i++;
	if (i > MAXIMUM_RAM_ADDRESSES)
		return;

	j=0;
	for (; i<MAXIMUM_RAM_VALUES; i++) {
		if (custom_addresses_values[i]==0)
			break;
		ram_values[j]=custom_addresses_values[i];
		j++;
	}
}


int handle_eeprom_data(struct var_eeprom_v2_cfg *var_eeprom_v2_cfg, \
		u32 *ram_addresses, u32 *ram_values)
{
	load_custom_data(var_eeprom_v2_cfg->custom_addresses_values, ram_addresses, ram_values);
	return setup_ddr_parameters(var_eeprom_v2_cfg->eeprom_commands, \
			ram_addresses, ram_values);
}


int setup_ddr_parameters(struct eeprom_command *eeprom_commands, \
		u32 *ram_addresses, u32 *ram_values)
{
	int i=0;

	while (i < MAXIMUM_COMMANDS_NUMBER) {
		i = handle_one_command(eeprom_commands, i, ram_addresses, ram_values);
		if (i < 0)
			return -1;
		if (i == 0)
			return 0;
	}
	return 0;
}


static int handle_one_command(struct eeprom_command *eeprom_commands, int command_num, \
		u32 *ram_addresses, u32 *ram_values)
{
	volatile u32 *data;
	u32 address;
	u32 value;

	eeprom_v2_debug("Executing command %03d: %03d,  %03d\n",
			command_num,
			eeprom_commands[command_num].address_index,
			eeprom_commands[command_num].value_index);

	switch(eeprom_commands[command_num].address_index) {
	case WHILE_NOT_EQUAL_INDEX:
		command_num++;
		address=get_address_by_index(eeprom_commands[command_num].address_index, ram_addresses);
		value=get_value_by_index(eeprom_commands[command_num].value_index, ram_values);
		data=(u32*)address;
		eeprom_v2_debug("Waiting while data at address %08x is not equal %08x\n", address, value);

		while(data[0]!=value);

		command_num++;
		break;
	case WHILE_EQUAL_INDEX:
		command_num++;
		address=get_address_by_index(eeprom_commands[command_num].address_index, ram_addresses);
		value=get_value_by_index(eeprom_commands[command_num].value_index, ram_values);
		data=(u32*)address;
		eeprom_v2_debug("Waiting while data at address %08x is equal %08x\n", address, value);

		while(data[0]==value);

		command_num++;
		break;
	case WHILE_AND_INDEX:
		command_num++;
		address=get_address_by_index(eeprom_commands[command_num].address_index, ram_addresses);
		value=get_value_by_index(eeprom_commands[command_num].value_index, ram_values);
		data=(u32*)address;
		eeprom_v2_debug("Waiting while data at address %08x and %08x is not zero\n", address, value);

		while(data[0]&value);

		command_num++;
		break;
	case WHILE_NOT_AND_INDEX:
		command_num++;
		address=get_address_by_index(eeprom_commands[command_num].address_index, ram_addresses);
		value=get_value_by_index(eeprom_commands[command_num].value_index, ram_values);
		data=(u32*)address;
		eeprom_v2_debug("Waiting while data at address %08x and %08x is zero\n", address, value);

		while(!(data[0]&value));

		command_num++;
		break;
	case DELAY_10USEC_INDEX:
		/* Delay for Value * 10 uSeconds */
		eeprom_v2_debug("Delaying for %d microseconds\n", eeprom_commands[command_num].value_index*10);
		udelay((int)(eeprom_commands[command_num].value_index*10));
		command_num++;
		break;
	case LAST_COMMAND_INDEX:
		command_num=0;
		break;
	default:
		address=get_address_by_index(eeprom_commands[command_num].address_index, ram_addresses);
		value=get_value_by_index(eeprom_commands[command_num].value_index, ram_values);
		data=(u32*)address;
		eeprom_v2_debug("Setting data at address %08x to %08x\n", address, value);
		data[0]=value;
		command_num++;
		break;
	}

	return command_num;
}


static u32 get_address_by_index(unsigned char index, u32 *ram_addresses)
{
	/*
	 * DDR Register struct
	 * The eeprom contains structure of
	 * 1 byte index in this table, 1 byte index to common values in the next table to write to this address.
	 * If there is some new addresses got from the calibration program they should be added in the end of the array.
	 * The maximum array size is 256 addresses.
	 */
	const u32 rom_addresses[]=
	{
		#include "addresses.inc"
	};

	if (index >= MAXIMUM_ROM_ADDR_INDEX)
		return ram_addresses[index-MAXIMUM_ROM_ADDR_INDEX];

	return rom_addresses[index];
}


static u32 get_value_by_index(unsigned char index, u32 *ram_values)
{
	const u32 rom_values[] =
	{
		#include "values.inc"
	};

	if (index >= MAXIMUM_ROM_VALUE_INDEX)
		return ram_values[index-MAXIMUM_ROM_VALUE_INDEX];

	return rom_values[index];
}


int var_eeprom_v2_read_struct(struct var_eeprom_v2_cfg *var_eeprom_v2_cfg, \
		unsigned char address)
{
	int eeprom_found = i2c_probe(address);
	if (0 == eeprom_found) {
		if (i2c_read(address, 0, 1, (void*) var_eeprom_v2_cfg, \
					sizeof(struct var_eeprom_v2_cfg))) {
			printf("Read device ID error!\n");
			return -1;
		}
	} else {
		printf("Error! Couldn't find EEPROM device\n");
	}
	return 0;
}
#endif
