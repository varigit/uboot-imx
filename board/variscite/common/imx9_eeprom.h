/*
 * Copyright (C) 2023 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX9_VAR_EEPROM_H_
#define _MX9_VAR_EEPROM_H_

#ifdef CONFIG_ARCH_IMX9
#include <asm/arch-imx9/ddr.h>
#endif

#define VAR_SOM_EEPROM_MAGIC	0x4D58 /* == HEX("MX") */

#define VAR_SOM_EEPROM_I2C_ADDR	0x52

/* Optional SOM features */
#define VAR_EEPROM_F_WIFI		(1 << 0)
#define VAR_EEPROM_F_ETH		(1 << 1)
#define VAR_EEPROM_F_AUDIO		(1 << 2)
#define VAR_EEPROM_F_WBE		(1 << 3)

/* Helpers to extract the major and minor versions from somrev */
#define SOMREV_MINOR(val) ((val) & GENMASK(4, 0))
#define SOMREV_MAJOR(val) (1 + (((val) >> 5) & GENMASK(2, 0)))

/* SOM storage types */
enum som_storage {
	SOM_STORAGE_EMMC,
	SOM_STORAGE_NAND,
	SOM_STORAGE_UNDEFINED,
};

/* Number of DRAM adjustment tables */
#define DRAM_TABLE_NUM 13
#define NUM_FSPS 3

struct __attribute__((packed)) var_eeprom
{
	u16 magic;			/* 00-0x00 - magic number       */
	u8 partnum[8];			/* 02-0x02 - part number        */
	u8 assembly[10];		/* 10-0x0a - assembly number    */
	u8 date[9];			/* 20-0x14 - build date         */
	u8 mac[6];			/* 29-0x1d - MAC address        */
	u8 somrev;			/* 35-0x23 - SOM revision       */
	u8 version;			/* 36-0x24 - EEPROM version     */
	u8 features;			/* 37-0x25 - SOM features       */
	u8 dramsize;			/* 38-0x26 - DRAM size          */
	u8 reserved[5];			/* 39 0x27 - reserved           */
	u32 ddr_crc32;			/* 44-0x2c - CRC32 of DDR DATAi */
	u16 ddr_vic;			/* 48-0x30 - DDR VIC PN         */
	u16 off[DRAM_TABLE_NUM+1];	/* 50-0x32 - DRAM table offsets */
	u16 fsp_drate[NUM_FSPS];	/* 78-0x4e - ddr_dram_fsp_msg[i].drate */
	u8 fsp_bypass;			/* 84-0x54 - Bitfield for ddr_dram_fsp_cfg[i].bypass */
};

#define VAR_EEPROM_DATA ((struct var_eeprom *)VAR_EEPROM_DRAM_START)

#define VAR_CARRIER_EEPROM_MAGIC	0x5643 /* == HEX("VC") */

#define CARRIER_REV_LEN 16
struct __attribute__((packed)) var_carrier_eeprom
{
	u16 magic;                          /* 00-0x00 - magic number		*/
	u8 struct_ver;                      /* 01-0x01 - EEPROM structure version	*/
	u8 carrier_rev[CARRIER_REV_LEN];    /* 02-0x02 - carrier board revision	*/
	u32 crc;                            /* 10-0x0a - checksum			*/
};

static inline int var_eeprom_is_valid(struct var_eeprom *ep)
{
	if (htons(ep->magic) != VAR_SOM_EEPROM_MAGIC) {
		debug("Invalid EEPROM magic 0x%hx, expected 0x%hx\n",
			htons(ep->magic), VAR_SOM_EEPROM_MAGIC);
		return 0;
	}

	return 1;
}

int var_eeprom_read_header(struct var_eeprom *e);
int var_eeprom_get_dram_size(struct var_eeprom *e, phys_size_t *size);
int var_eeprom_get_mac(struct var_eeprom *e, u8 *mac);
void var_eeprom_print_prod_info(struct var_eeprom *e);

#if defined(CONFIG_SPL_BUILD)
void var_eeprom_adjust_dram(struct var_eeprom *e, struct dram_timing_info *d);
#endif

int var_carrier_eeprom_read(const char * bus_name, int addr, struct var_carrier_eeprom *ep);
int var_carrier_eeprom_is_valid(struct var_carrier_eeprom *ep);
void var_carrier_eeprom_get_revision(struct var_carrier_eeprom *ep, char *rev, size_t size);

#endif /* _MX9_VAR_EEPROM_H_ */
