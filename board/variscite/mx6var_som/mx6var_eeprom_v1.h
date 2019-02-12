/*
 * Copyright (C) 2016 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX6VAR_EEPROM_V1_H_
#define _MX6VAR_EEPROM_V1_H_

#define VARISCITE_MAGIC			0x49524157 /* == HEX("VARI")*/

#define VAR_EEPROM_I2C_BUS		1
#define VAR_EEPROM_I2C_ADDR		0x56
#define VAR_EEPROM_SIZE_BYTES		512
#define VAR_EEPROM_MAX_NUM_OF_OPCODES	((VAR_EEPROM_SIZE_BYTES \
						- sizeof(struct var_eeprom_v1_cfg_header) \
						- sizeof(struct var_pinmux_group_regs)) \
						/ sizeof(struct reg_write_opcode))

#define VAR_EEPROM_WRITE_MAX_SIZE	0x4

#define VAR_DDR_INIT_OPCODE_WRITE_VAL_TO_ADDRESS	0x00000000
#define VAR_DDR_INIT_OPCODE_WAIT_MASK_RISING		0x10000000
#define VAR_DDR_INIT_OPCODE_WAIT_MASK_FALLING		0x20000000
#define VAR_DDR_INIT_OPCODE_DELAY_USEC			0x30000000

#ifdef EEPROM_V1_DEBUG
#define eeprom_v1_debug(M, ...) printf("EEPROM_V1 DEBUG: " M, ##__VA_ARGS__)
#else
#define eeprom_v1_debug(M, ...)
#endif

struct var_eeprom_v1_cfg_header
{
	u32 variscite_magic; /* == HEX("VARI")?*/
	u8 part_number[16];
	u8 assembly[16];
	u8 date[16];
	u8 version;
	u8 reserved[7];
	/* DRAM size in MiB */
	u32 dram_size;
};

struct var_pinmux_group_regs
{
	u32 IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE;
	u32 IOMUXC_SW_PAD_CTL_GRP_DDRPKE;
	u32 IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL;
	u32 IOMUXC_SW_PAD_CTL_GRP_DDRMODE;
	u32 IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2;
	u32 dram_reset;
	u32 dram_sdcke0;
	u32 dram_sdcke1;
	u32 dram_sdodt0;
	u32 dram_sdodt1;
	u32 reserved;
	u32 pinmux_ctrlpad_all_value;
};

struct reg_write_opcode
{
	u32 address; /* address encoded with opcode */
	u32 value;
};

struct var_eeprom_v1_cfg
{
	struct var_eeprom_v1_cfg_header header;
	struct var_pinmux_group_regs pinmux_group;
	struct reg_write_opcode write_opcodes[VAR_EEPROM_MAX_NUM_OF_OPCODES];
};

#endif /* _MX6VAR_EEPROM_V1_H_ */
