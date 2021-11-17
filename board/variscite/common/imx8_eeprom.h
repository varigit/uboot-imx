/*
 * Copyright (C) 2018-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX8_VAR_EEPROM_H_
#define _MX8_VAR_EEPROM_H_

#ifdef CONFIG_ARCH_IMX8M
#include <asm/arch-imx8m/ddr.h>
#endif

#define VAR_EEPROM_MAGIC	0x384D /* == HEX("8M") */

#define VAR_EEPROM_I2C_BUS	0
#define VAR_EEPROM_I2C_ADDR	0x52

/* Optional SOM features */
#define VAR_EEPROM_F_WIFI		(1 << 0)
#define VAR_EEPROM_F_ETH		(1 << 1)
#define VAR_EEPROM_F_AUDIO		(1 << 2)
#define VAR_EEPROM_F_MX8M_LVDS		(1 << 3) /* i.MX8MM, i.MX8MN, i.MX8MQ only */
#define VAR_EEPROM_F_MX8Q_SOC_ID	(1 << 3) /* 0 = i.MX8QM, 1 = i.MX8QP */
#define VAR_EEPROM_F_NAND		(1 << 4)

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

#define VAR_EEPROM_DATA ((struct var_eeprom *)VAR_EEPROM_DRAM_START)

#define VAR_CARRIER_EEPROM_MAGIC	0x5643 /* == HEX("VC") */

struct __attribute__((packed)) var_carrier_eeprom
{
	u16 magic;		/* 00-0x00 - magic number		*/
	u8 struct_ver;		/* 01-0x01 - EEPROM structure version	*/
	u8 carrier_rev[16];	/* 02-0x02 - carrier board revision	*/
	u32 crc;		/* 10-0x0a - checksum			*/
};

static inline int var_eeprom_is_valid(struct var_eeprom *ep)
{
	if (htons(ep->magic) != VAR_EEPROM_MAGIC) {
		debug("Invalid EEPROM magic 0x%hx, expected 0x%hx\n",
			htons(ep->magic), VAR_EEPROM_MAGIC);
		return 0;
	}

	return 1;
}

int var_eeprom_read_header(struct var_eeprom *e);
int var_scu_eeprom_read_header(struct var_eeprom *e);
int var_eeprom_get_dram_size(struct var_eeprom *e, phys_size_t *size);
int var_eeprom_get_mac(struct var_eeprom *e, u8 *mac);
int var_eeprom_get_storage(struct var_eeprom *e, int *storage);
void var_eeprom_print_prod_info(struct var_eeprom *e);

#if defined(CONFIG_ARCH_IMX8M) && defined(CONFIG_SPL_BUILD)
void var_eeprom_adjust_dram(struct var_eeprom *e, struct dram_timing_info *d);
#endif

int var_carrier_eeprom_read(int bus, int addr, struct var_carrier_eeprom *ep);
int var_carrier_eeprom_is_valid(struct var_carrier_eeprom *ep);
void var_carrier_eeprom_get_revision(struct var_carrier_eeprom *ep, char *rev, size_t size);

#endif /* _MX8M_VAR_EEPROM_H_ */
