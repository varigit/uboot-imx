/*
 * Copyright (C) 2018 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX8M_VAR_EEPROM_H_
#define _MX8M_VAR_EEPROM_H_

#define VAR_EEPROM_MAGIC	0x384D /* == HEX("8M") */

#define VAR_EEPROM_I2C_BUS	0
#define VAR_EEPROM_I2C_ADDR	0x52
#define VAR_EEPROM_ADDR_LEN	1

#ifdef EEPROM_DEBUG
#define eeprom_debug(M, ...) printf("EEPROM_DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_debug(M, ...)
#endif


struct __attribute__((packed)) var_eeprom
{
	u16 magic;
	u8 partnum[8];
	u8 assembly[11];
	u8 date[9];
	u8 mac[6];
	u8 revision;
	u8 features;
	u8 dram_size;
};

extern int var_eeprom_read_mac(u8 *buf);
extern int var_eeprom_read_dram_size(u8 *buf);

#endif /* _MX8M_VAR_EEPROM_H_ */
