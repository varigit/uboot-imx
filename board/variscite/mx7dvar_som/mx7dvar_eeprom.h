/*
 * Copyright (C) 2018 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX7DVAR_EEPROM_H_
#define _MX7DVAR_EEPROM_H_

#define EEPROM_MAGIC		0x3744 /* == HEX("7D") */

#define EEPROM_I2C_BUS		0
#define EEPROM_I2C_ADDR		0x50

#ifdef EEPROM_DEBUG
#define eeprom_debug(M, ...) printf("EEPROM DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_debug(M, ...)
#endif

struct __attribute__((packed)) var_eeprom_cfg
{
	u16 eeprom_magic; /* == EEPROM_MAGIC ? */
	u8 part_number[8];
	u8 assembly[11];
	u8 date[9];
	u8 reserved;
	u8 dram_size; /* in 128MB units */
	u32 dcd_table[120];
};

#endif /* _MX7DVAR_EEPROM_H_ */
