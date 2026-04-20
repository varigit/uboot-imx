/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) 2018-2025 Variscite Ltd.
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
#define VAR_EEPROM_F_WIFI		BIT(0)	/* Wi-Fi assembled; WBD if WBE/WBK are clear */
#define VAR_EEPROM_F_ETH		BIT(1)	/* EC1 GbE PHY on first Ethernet port */
#define VAR_EEPROM_F_AUDIO		BIT(2)
#define VAR_EEPROM_F_MX8M_LVDS		BIT(3)	/* i.MX8MM, i.MX8MN, i.MX8MQ only */
#define VAR_EEPROM_F_MX8Q_SOC_ID	BIT(3)	/* 0 = i.MX8QM, 1 = i.MX8QP */
#define VAR_EEPROM_F_NAND		BIT(4)
#define VAR_EEPROM_F_WBE		BIT(5)	/* Wi-Fi WBE ordering option */
#define VAR_EEPROM_F_WBK		BIT(6)	/* Wi-Fi WBK ordering option */

/* Additional optional SOM features for VAR-SMARC-MX8M-PLUS */
#define VAR_EEPROM_F2_ETH2		BIT(0)	/* EC2: GbE PHY on second Ethernet port */
#define VAR_EEPROM_F2_HUB		BIT(1)	/* HUB: USB hub included */
#define VAR_EEPROM_F2_RTC		BIT(2)	/* RTC: Internal RTC */
#define VAR_EEPROM_F2_DSI		BIT(3)	/* DSI: DSI output instead of LVDS #0 */
#define VAR_EEPROM_F2_TPMST		BIT(4)	/* TPMST: internal ST TPM secure component */
#define VAR_EEPROM_F2_TPMIF		BIT(5)	/* TPMIF: internal Infineon TPM secure component */

/*
 * EEPROM format version thresholds.
 *
 * Add entries here when a specific EEPROM version introduces
 * behavior or fields that need to be checked in code.
 */
#define VAR_EEPROM_VER_FEATURES2	5	/* features2 introduced and factory-programmed */

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
#define DRAM_TABLE_NUM 7

struct __packed var_eeprom
{
	u16 magic;			/* 00-0x00 - magic number       */
	u8 partnum[3];			/* 02-0x02 - part number        */
	u8 assembly[10];		/* 05-0x05 - assembly number    */
	u8 date[9];			/* 15-0x0f - build date         */
	u8 mac[6];			/* 24-0x18 - MAC address        */
	u8 somrev;			/* 30-0x1e - SOM revision       */
	u8 version;			/* 31-0x1f - EEPROM version     */
	u8 features;			/* 32-0x20 - SOM features       */
	u8 dramsize;			/* 33-0x21 - DRAM size          */
	u8 off[DRAM_TABLE_NUM + 1];	/* 34-0x22 - DRAM table offsets */
	u8 partnum2[5];			/* 42-0x2a - part number        */
	u8 features2;			/* 47-0x2f - SOM features 2     */
	u8 reserved[2];			/* 48-0x30 - reserved           */
};

#define VAR_EEPROM_DATA ((struct var_eeprom *)VAR_EEPROM_DRAM_START)
#define VAR_CARRIER_EEPROM_DATA ((struct var_carrier_eeprom *)(VAR_EEPROM_DRAM_START + \
							       sizeof(struct var_eeprom)))

#define VAR_CARRIER_EEPROM_MAGIC	0x5643 /* == HEX("VC") */

#define CARRIER_REV_LEN 16
struct __packed var_carrier_eeprom
{
	u16 magic;                          /* 00-0x00 - magic number		*/
	u8 struct_ver;                      /* 01-0x01 - EEPROM structure version	*/
	u8 carrier_rev[CARRIER_REV_LEN];    /* 02-0x02 - carrier board revision	*/
	u32 crc;                            /* 10-0x0a - checksum			*/
};

struct dram_fixup_param {
	unsigned int dramsize;
	unsigned int reg;
	unsigned int old_val;
	unsigned int new_val;
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
void var_eeprom_apply_dram_fixup(struct var_eeprom *ep, struct dram_fixup_param *fixup_regs,
			    struct dram_cfg_param *table, int table_size);
#endif

int var_carrier_eeprom_read(int bus, int addr, struct var_carrier_eeprom *ep);
int var_carrier_eeprom_is_valid(struct var_carrier_eeprom *ep);
void var_carrier_eeprom_get_revision(struct var_carrier_eeprom *ep, char *rev, size_t size);
int var_carrier_eeprom_get_name(struct var_carrier_eeprom *ep, char *name);

#endif /* _MX8M_VAR_EEPROM_H_ */
