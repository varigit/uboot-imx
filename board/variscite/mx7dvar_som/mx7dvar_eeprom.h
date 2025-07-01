/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018-2025 Variscite Ltd.
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

struct __packed mx7d_var_legacy_eeprom {
	u32 reserved;
	u8 part_number[16];
	u8 assembly[16];
	u8 date_year[4];
	u8 date_month[3];
	u8 date_day[2];
};

struct __packed mx7d_var_eeprom {
	u16 eeprom_magic;
	u8 part_number[8];
	u8 assembly[11];
	u8 date_year[4];
	u8 date_month[3];
	u8 date_day[2];
	u8 reserved;
	u8 dram_size;		/* DRAM size in 128MB units */
	u32 dcd_table[120];
};

#define VAR_EEPROM_DATA ((struct mx7d_var_eeprom *)VAR_EEPROM_DRAM_START)

void mx7d_var_eeprom_print_legacy_production_info(const struct mx7d_var_eeprom *e);
void mx7d_var_eeprom_print_production_info(const struct mx7d_var_eeprom *e);
int mx7d_var_eeprom_read_header(struct mx7d_var_eeprom *e);

static inline bool mx7d_var_eeprom_is_valid(const struct mx7d_var_eeprom *e)
{
	return (e->eeprom_magic == EEPROM_MAGIC);
}
#endif /* _MX7DVAR_EEPROM_H_ */
