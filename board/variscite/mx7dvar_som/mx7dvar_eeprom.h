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
	u32 reserved;		/* 00 - 0x00 - reserved */
	u8 part_number[16];	/* 04 - 0x04 - part number suffix */
	u8 assembly[16];	/* 20 - 0x14 - assembly number suffix */
	u8 date_year[4];	/* 36 - 0x24 - production year */
	u8 date_month[3];	/* 40 - 0x28 - production month */
	u8 date_day[2];		/* 43 - 0x2b - production day */
};

struct __packed mx7d_var_eeprom {
	u16 eeprom_magic;	/* 00 - 0x00 - magic ID, must be EEPROM_MAGIC */
	u8 part_number[8];	/* 02 - 0x02 - part number suffix */
	u8 assembly[11];	/* 10 - 0x0a - assembly number suffix */
	u8 date_year[4];	/* 21 - 0x15 - production year */
	u8 date_month[3];	/* 25 - 0x19 - production month */
	u8 date_day[2];		/* 28 - 0x1c - production day */
	u8 reserved;		/* 30 - 0x1e - reserved */
	u8 dram_size;		/* 31 - 0x1f - DRAM size in 128MB units */
	u32 dcd_table[120];	/* 32 - 0x20 - DRAM DCD register table */
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
