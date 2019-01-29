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
#define VAR_EEPROM_PAGE_SIZE	16

/* Optional SOM features */
#define VAR_EEPROM_F_WIFI 	(1 << 0)
#define VAR_EEPROM_F_ETH 	(1 << 1)
#define VAR_EEPROM_F_AUDIO 	(1 << 2)
#define VAR_EEPROM_F_LVDS	(1 << 3)

/* SOM revision numbers */
#define SOM_REV_1_1		0

#ifdef EEPROM_DEBUG
#define eeprom_debug(M, ...) printf("EEPROM_DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_debug(M, ...)
#endif

struct __attribute__((packed)) var_eeprom
{
	u16 magic;	/* 00-0x00 - magic number    -	8M	        */
	u8 pn[3];	/* 02-0x02 - part number     -	VSM-DT8M-001    */
	u8 as[10];	/* 05-0x05 - assembly        -	AS1803138551    */
	u8 date[9];	/* 15-0x0f - build date      -	YYYYMMMDD       */
	u8 mac[6];	/* 24-0x18 - MAC address     -	000102030405    */
	u8 sr;		/* 30-0x1e - SOM revision    -	01              */
	u8 ver;		/* 31-0x1f - EEPROM version  -	01              */
	u8 opt;		/* 32-0x20 - SOM features    -	0f              */
	u8 rs;		/* 33-0x21 - RAM size in GB  -	01              */
};

extern int var_eeprom_read_mac(u8 *buf);
extern int var_eeprom_read_dram_size(u8 *buf);
extern void var_eeprom_print_info(void);

#endif /* _MX8M_VAR_EEPROM_H_ */
