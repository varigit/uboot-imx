/*
 * Copyright (C) 2015 Variscite Ltd. All Rights Reserved.
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
#ifndef _VAR_V2_EEPROM_H_
#define _VAR_V2_EEPROM_H_

#define MAXIMUM_ROM_ADDR_INDEX		200
#define MAXIMUM_CUSTOM_INDEX		240
#define WHILE_NOT_EQUAL_INDEX		241
#define WHILE_EQUAL_INDEX			242
#define WHILE_AND_INDEX				243
#define WHILE_NOT_AND_INDEX			244
#define DELAY_10USEC_INDEX			245
#define LAST_COMMAND_INDEX			255

#define MAXIMUM_ROM_VALUE_INDEX		200

#define MAXIMUM_RAM_ADDRESSES		32
#define MAXIMUM_RAM_VALUES			32

#define MAXIMUM_COMMANDS_NUMBER	150

#define VARISCITE_MAGIC 0x49524157 /* == HEX("VARI") */

#define VARISCITE_MX6_EEPROM_CHIP 0x50

#define VARISCITE_MX6_EEPROM_STRUCT_OFFSET 0x00000000

#define VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE 0x4

#define EEPROM_SIZE_BYTES 512

#define SPL_DRAM_INIT_STATUS_GEN_ERROR				-1
#define	SPL_DRAM_INIT_STATUS_OK					0
#define	SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM			1
#define	SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED	2

extern void p_udelay(int time);

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


extern int handle_eeprom_data(struct var_eeprom_config_struct_v2_type *var_eeprom_config_struct_v2);
extern int setup_ddr_parameters(struct eeprom_command_type *eeprom_commands);
extern void load_custom_data(u32 *custom_addresses_values);

#endif /* _VAR_V2_EEPROM_H_ */
