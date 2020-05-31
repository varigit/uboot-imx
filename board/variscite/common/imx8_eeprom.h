/*
 * Copyright (C) 2018-2019 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX8_VAR_EEPROM_H_
#define _MX8_VAR_EEPROM_H_

#define VAR_EEPROM_MAGIC	0x384D /* == HEX("8M") */

#define VAR_EEPROM_I2C_BUS	0
#define VAR_EEPROM_I2C_ADDR	0x52

/* Optional SOM features */
#define VAR_EEPROM_F_WIFI 	(1 << 0)
#define VAR_EEPROM_F_ETH 	(1 << 1)
#define VAR_EEPROM_F_AUDIO 	(1 << 2)
#define VAR_EEPROM_F_LVDS	(1 << 3)
#define VAR_EEPROM_F_NAND	(1 << 4)

/* SOM revision numbers */
#define SOM_REV_1_1		0
#define SOM_REV_1_2		1

/* SOM storage types */
enum som_storage {
	SOM_STORAGE_EMMC,
	SOM_STORAGE_NAND,
	SOM_STORAGE_UNDEFINED,
};

/* Number of DRAM adjustment tables */
#define DRAM_TABLE_NUM 7

struct __attribute__((packed)) var_eeprom
{
	u16 magic;                /* 00-0x00 - magic number       */
	u8 partnum[3];            /* 02-0x02 - part number        */
	u8 assembly[10];          /* 05-0x05 - assembly number    */
	u8 date[9];               /* 15-0x0f - build date         */
	u8 mac[6];                /* 24-0x18 - MAC address        */
	u8 somrev;                /* 30-0x1e - SOM revision       */
	u8 version;               /* 31-0x1f - EEPROM version     */
	u8 features;              /* 32-0x20 - SOM features       */
	u8 dramsize;              /* 33-0x21 - DRAM size          */
	u8 off[DRAM_TABLE_NUM+1]; /* 34-0x22 - DRAM table offsets */
	u8 partnum2[5];           /* 42-0x2a - part number        */
	u8 reserved[3];           /* 47 0x2f - reserved           */
};

static inline int var_eeprom_is_valid(struct var_eeprom *e)
{
	if (htons(e->magic) != VAR_EEPROM_MAGIC) {
		debug("Invalid EEPROM magic 0x%hx, expected 0x%hx\n",
			htons(e->magic), VAR_EEPROM_MAGIC);
		return 0;
	}

	return 1;
}

extern int var_eeprom_read_header(struct var_eeprom *e);
extern int var_eeprom_get_dram_size(struct var_eeprom *e, u32 *size);
extern int var_eeprom_get_mac(struct var_eeprom *e, u8 *mac);
extern int var_eeprom_get_storage(struct var_eeprom *e, int *storage);
extern void var_eeprom_print_prod_info(struct var_eeprom *e);

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_ARCH_IMX8M)
extern void var_eeprom_adjust_dram(struct var_eeprom *e, struct dram_timing_info *d);
#endif

#endif /* _MX8M_VAR_EEPROM_H_ */
