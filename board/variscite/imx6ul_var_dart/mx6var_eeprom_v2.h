/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 */

#ifndef _VAR_EEPROM_V2_H_
#define _VAR_EEPROM_V2_H_

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

#define DART6UL_INFO_STORAGE_GET(n) ((n) & 0x3)
#define DART6UL_INFO_WIFI_GET(n)    ((n) >> 2 & 0x1)
#define DART6UL_INFO_REV_GET(n)     ((n) >> 3 & 0x3)
#define DART6UL_DDRSIZE(n)          ((n) * SZ_128M)
#define DART6UL_INFO_MAGIC          0x32524156

#define DART6UL_PN_LEN   16
#define DART6UL_ASSY_LEN 16
#define DART6UL_DATE_LEN 12

/* eeprom content, 512 bytes */
struct __packed var_eeprom {
	u32 magic;
	u8 partnumber[DART6UL_PN_LEN];
	u8 assy[DART6UL_ASSY_LEN];
	u8 date[DART6UL_DATE_LEN];
	u32 custom_addr_val[32];
	struct cmd {
		u8 addr;
		u8 index;
	} custom_cmd[150];
	u8 res[33];
	u8 som_info;
	u8 ddr_size;
	u8 crc;
};

#define VAR_EEPROM_DATA ((struct var_eeprom *)VAR_EEPROM_DRAM_START)

const char *som_info_storage_to_str(u8 som_info);
const char *som_info_rev_to_str(u8 som_info);
void var_eeprom_print_prod_infos(struct var_eeprom *e);
int var_eeprom_dram_init(struct var_eeprom *e);
int var_eeprom_read_header(struct var_eeprom *e);

static inline bool var_eeprom_is_valid(const struct var_eeprom *e)
{
	return (e->magic == DART6UL_INFO_MAGIC);
}

#endif /* _VAR_EEPROM_V2_H_ */
