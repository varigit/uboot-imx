/*
 * Copyright (C) 2018 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX7DVAR_EEPROM_H_
#define _MX7DVAR_EEPROM_H_

#include <linux/bitops.h>

#define EEPROM_MAGIC		0x3744 /* == HEX("7D") */
#define EEPROM_MAGIC_V2		0x3745 /* == HEX("7D") + 1 */

#define EEPROM_I2C_BUS		0
#define EEPROM_I2C_ADDR		0x50

#ifdef EEPROM_DEBUG
#define eeprom_debug(M, ...) printf("EEPROM DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_debug(M, ...)
#endif

/*
 * EEPROM SOM Info Bits Description
 *
 * Bit(s)   Description
 * ------------------------------------------------------------------
 * 0-5      Reserved (0)
 *
 * 6-7      Wi-Fi
 *              00 = No Wi-Fi / BT
 *              01 = WBD - IW611
 */
#define VAR_EEPROM_WIFI_MASK		GENMASK(7, 6)

#define VAR_EEPROM_WIFI_NONE		0x0	/* No Wi-Fi / BT chipset assembled */
#define VAR_EEPROM_WIFI_WBD		0x1	/* IW611 */

struct __attribute__((packed)) var_eeprom_cfg
{
	u16 eeprom_magic;	/* 00 - 0x00 - magic ID, must be EEPROM_MAGIC / EEPROM_MAGIC_V2 */
	u8 part_number[8];	/* 02 - 0x02 - part number suffix */
	u8 assembly[11];	/* 10 - 0x0a - assembly number suffix */
	u8 date[9];		/* 21 - 0x15 - production date (YYYYMMMDD) */
	u8 som_info;		/* 30 - 0x1e - SoM information (only available for EEPROM v2, otherwise not used) */
	u8 dram_size;		/* 31 - 0x1f - DRAM size in 128MB units */
	u32 dcd_table[120];	/* 32 - 0x20 - DRAM DCD register table */
};

static inline bool var_eeprom_is_valid_v1(u16 eeprom_magic)
{
	return (eeprom_magic == EEPROM_MAGIC);
}

static inline bool var_eeprom_is_valid_v2(u16 eeprom_magic)
{
	return (eeprom_magic == EEPROM_MAGIC_V2);
}

static inline bool var_eeprom_is_valid(u16 eeprom_magic)
{
	return var_eeprom_is_valid_v1(eeprom_magic) ||
		var_eeprom_is_valid_v2(eeprom_magic);
}

#endif /* _MX7DVAR_EEPROM_H_ */
