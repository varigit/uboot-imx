/*
 * Copyright (C) 2015-2024 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _VAR_EEPROM_V2_H_
#define _VAR_EEPROM_V2_H_

#define VARISCITE_MAGIC_V2		0x32524156 /* == HEX("VAR2") */

#define VAR_DART_EEPROM_I2C_BUS		1
#define VAR_DART_EEPROM_I2C_ADDR	0x50

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

#define VAR_EEPROM_WRITE_MAX_SIZE	0x4

#ifdef EEPROM_V2_DEBUG
#define eeprom_v2_debug(M, ...) printf("EEPROM_V2 DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_v2_debug(M, ...)
#endif

/*
 * EEPROM command structure (2 bytes per command). This structure holds
 * an index into the address and value tables for EEPROM commands.
 */
struct __attribute__((packed)) eeprom_command
{
	/* 00-0x00: Index into the address table (1 byte) */
	u8 address_index;
	/* 01-0x01: Index into the value table (1 byte) */
	u8 value_index;
};

/*
 * EEPROM SOM Info Bits Description
 *
 * Bit(s)   Description
 * ------------------------------------------------------------------------
 * 0-1      Storage Type
 *              00 = SD card only
 *              01 = NAND flash
 *              10 = eMMC
 *              11 = Illegal
 *
 * 2        Wi-Fi
 *              0 = No Wi-Fi
 *              1 = Wi-Fi
 *
 * 3-4      SOM Revision / Configuration
 *              00 = DART-6UL
 *              01 = DART-6UL-5G
 *              10 = DART-6UL-5G (IW611)
 *              11 = DART-6UL-5G (IW612)
 */
struct __attribute__((packed)) var_eeprom_v2_cfg
{
	/* 00-0x00: EEPROM Magic number (4 bytes) == HEX("VAR2")? */
	u32 variscite_magic;
	/* 04-0x04: Part number string (16 bytes) */
	u8 part_number[16];
	/* 20-0x14: Assembly number string (16 bytes) */
	u8 assembly[16];
	/* 36-0x24: Build date string (12 bytes) */
	u8 date[12];
	/* 48-0x30: Custom addresses/values (128 bytes)
	 * Contains addresses and values not present in .inc files
	 */
	u32 custom_addresses_values[32];
	/* 176-0xB0: EEPROM commands array (300 bytes) */
	struct eeprom_command commands[MAX_NUM_OF_COMMANDS];
	/* 476-0x1DC: Reserved space for future use (33 bytes) */
	u8 reserved[33];
	/* 509-0x1FD: SOM information (1 byte) */
	u8 som_info;
	/* 510-0x1FE: DRAM size in 128MiB units (1 byte) */
	u8 dram_size;
	/* 511-0x1FF: CRC for data integrity (1 byte) */
	u8 crc;
};

int var_eeprom_v2_read_struct(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg);
int var_eeprom_v2_dram_init(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg);
void var_eeprom_v2_print_production_info(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg);
void var_eeprom_v2_print_som_info(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg);

#endif /* _VAR_EEPROM_V2_H_ */
