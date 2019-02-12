/*
 * Copyright (C) 2016 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX6VAR_EEPROM_V2_H_
#define _MX6VAR_EEPROM_V2_H_

#define VARISCITE_MAGIC_V2		0x32524156 /* == HEX("VAR2") */

#define VAR_DART_EEPROM_I2C_BUS		1
#define VAR_DART_EEPROM_I2C_ADDR	0x52

#define WHILE_NOT_EQUAL_INDEX		241
#define WHILE_EQUAL_INDEX		242
#define WHILE_AND_INDEX			243
#define WHILE_NOT_AND_INDEX		244
#define DELAY_10USEC_INDEX		245
#define LAST_COMMAND_INDEX		255

#define MAX_CUSTOM_ADDRESSES		32
#define MAX_CUSTOM_VALUES		32

#define MAX_COMMON_ADDRS_INDEX		200
#define MAX_COMMON_VALUES_INDEX		200

#define MAX_NUM_OF_COMMANDS		150

#ifdef EEPROM_V2_DEBUG
#define eeprom_v2_debug(M, ...) printf("EEPROM_V2 DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_v2_debug(M, ...)
#endif

struct __attribute__((packed)) eeprom_command
{
	u8 address_index;
	u8 value_index;
};

struct __attribute__((packed)) var_eeprom_v2_cfg
{
	u32 variscite_magic; /* == HEX("VAR2")? */
	u8 part_number[16];
	u8 assembly[16];
	u8 date[12];
	/* Contains addresses and values not present in .inc files */
	u32 custom_addresses_values[32];
	struct eeprom_command commands[MAX_NUM_OF_COMMANDS];
	u8 reserved[34];
	/* DRAM size in 8KiB unit */
	u8 dram_size;
	u8 crc;
};

#endif /* _MX6VAR_EEPROM_V2_H_ */
