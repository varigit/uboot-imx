/*
 * Copyright (C) 2015 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#ifndef _VAR_V2_EEPROM_H_
#define _VAR_V2_EEPROM_H_

#define MAXIMUM_ROM_ADDR_INDEX		200
#define MAXIMUM_CUSTOM_INDEX		240
#define WHILE_NOT_EQUAL_INDEX		241
#define WHILE_EQUAL_INDEX		242
#define WHILE_AND_INDEX			243
#define WHILE_NOT_AND_INDEX		244
#define DELAY_10USEC_INDEX		245
#define LAST_COMMAND_INDEX		255

#define MAXIMUM_ROM_VALUE_INDEX		200

#define MAXIMUM_RAM_ADDRESSES		32
#define MAXIMUM_RAM_VALUES		32

#define MAXIMUM_COMMANDS_NUMBER		150

#define VARISCITE_MAGIC_V2		0x32524156 /* == HEX("VAR2") */

#define VARISCITE_MX6_EEPROM_CHIP	0x50

#define VARISCITE_MX6_EEPROM_STRUCT_OFFSET	0x00000000

#define VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE	0x4

#define EEPROM_SIZE_BYTES		512

typedef struct __attribute__((packed)) eeprom_command_type
{
	unsigned char address_index;
	unsigned char value_index;
}eeprom_command_type;

typedef struct __attribute__((packed)) var_eeprom_config_struct_v2_type
{
	u32 variscite_magic; /* == HEX("VAR2")? */
	u8 part_number[16];
	u8 Assembly[16];
	u8 date[12];
	u32 custom_addresses_values[32];
	struct eeprom_command_type eeprom_commands[MAXIMUM_COMMANDS_NUMBER];
	u8 reserved[33];
	u8 som_info;	/* 0x1=NAND flash 0x2=eMMC 0x4 WiFi included */
	u8 ddr_size;
	u8 crc;
} var_eeprom_config_struct_v2_type;

int var_eeprom_v2_read_struct(struct var_eeprom_config_struct_v2_type *var_eeprom_config_struct_v2);
int handle_eeprom_data(struct var_eeprom_config_struct_v2_type *var_eeprom_config_struct_v2);

#endif /* _VAR_V2_EEPROM_H_ */
