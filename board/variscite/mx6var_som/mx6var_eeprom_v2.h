/*
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 * Maintainer: Eran Matityahu <eran.m@variscite.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef _MX6VAR_V2_EEPROM_H_
#define _MX6VAR_V2_EEPROM_H_

#define VARISCITE_MAGIC_V2 	0x32524156 /* == HEX("VAR2") */

#define MAXIMUM_ROM_ADDR_INDEX		200
#define MAXIMUM_CUSTOM_INDEX 		240
#define WHILE_NOT_EQUAL_INDEX		241
#define WHILE_EQUAL_INDEX		242
#define WHILE_AND_INDEX			243
#define WHILE_NOT_AND_INDEX		244
#define DELAY_10USEC_INDEX		245
#define LAST_COMMAND_INDEX		255

#define MAXIMUM_ROM_VALUE_INDEX		200

#define MAXIMUM_RAM_ADDRESSES		32
#define MAXIMUM_RAM_VALUES		32

#define MAXIMUM_COMMANDS_NUMBER 	150

struct __attribute__((packed)) eeprom_command
{
	unsigned char address_index;
	unsigned char value_index;
};

struct __attribute__((packed)) var_eeprom_v2_cfg
{
	u32 variscite_magic; /* == HEX("VAR2")? */
	u8 part_number[16];
	u8 Assembly[16];
	u8 date[12];
	u32 custom_addresses_values[32];
	struct eeprom_command eeprom_commands[MAXIMUM_COMMANDS_NUMBER];
	u8 reserved[34];
	u8 ddr_size;
	u8 crc;
};

bool var_eeprom_v2_is_valid(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg);
void var_eeprom_v2_strings_print(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg);
int handle_eeprom_data(struct var_eeprom_v2_cfg *var_eeprom_v2_cfg, \
		u32 *ram_addresses, u32 *ram_values);
int setup_ddr_parameters(struct eeprom_command *eeprom_commands, \
		u32 *ram_addresses, u32 *ram_values);
void load_custom_data(u32 *custom_addresses_values, u32 *ram_addresses, u32 *ram_values);
int var_eeprom_v2_read_struct(struct var_eeprom_v2_cfg *var_eeprom_v2_cfg, \
		unsigned char address);

#endif /* _MX6VAR_V2_EEPROM_H_ */
