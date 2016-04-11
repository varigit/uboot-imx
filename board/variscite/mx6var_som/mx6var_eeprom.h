/*
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX6VAR_EEPROM_H_
#define _MX6VAR_EEPROM_H_

#define VARISCITE_MAGIC 0x49524157 /* == HEX("VARI")*/

#define VAR_MX6_EEPROM_CHIP	0x56
#define VAR_DART_EEPROM_CHIP	0x52

#define VAR_MX6_EEPROM_STRUCT_OFFSET 0x00000000

#define VAR_MX6_EEPROM_WRITE_MAX_SIZE 0x4

#define EEPROM_SIZE_BYTES 512

#define VAR_DDR_INIT_OPCODE_WRITE_VAL_TO_ADDRESS 	0x00000000
#define VAR_DDR_INIT_OPCODE_WAIT_MASK_RISING 		0x10000000
#define VAR_DDR_INIT_OPCODE_WAIT_MASK_FALLING 		0x20000000
#define VAR_DDR_INIT_OPCODE_DELAY_USEC 			0x30000000

#define	SPL_DRAM_INIT_STATUS_OK 				0
#define	SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM			1
#define	SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED	2

struct var_eeprom_cfg_header
{
	u32 variscite_magic; /* == HEX("VARI")?*/
	u8 part_number[16];
	u8 Assembly[16];
	u8 date[16];
	u8 version;
	u8 reserved[7];
	u32 ddr_size;
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

struct var_eeprom_cfg
{
	struct var_eeprom_cfg_header header;
	struct var_pinmux_group_regs pinmux_group;
	struct reg_write_opcode write_opcodes[ \
		(EEPROM_SIZE_BYTES \
		 - sizeof(struct var_eeprom_cfg_header) \
		 - sizeof(struct var_pinmux_group_regs)) \
		/ sizeof(struct reg_write_opcode) ];
};

bool var_eeprom_is_valid(struct var_eeprom_cfg *p_var_eeprom_cfg);

/* init ddr iomux from struct */
void var_eeprom_mx6dlsl_dram_setup_iomux_from_struct(struct var_pinmux_group_regs *pinmux_group);

void var_eeprom_mx6qd_dram_setup_iomux_from_struct(struct var_pinmux_group_regs *pinmux_group);

/* init ddr from struct */
void var_eeprom_dram_init_from_struct(struct var_eeprom_cfg *p_var_eeprom_cfg);

int var_eeprom_read_struct(struct var_eeprom_cfg *p_var_eeprom_cfg);

void var_eeprom_strings_print(struct var_eeprom_cfg *p_var_eeprom_cfg);

#endif /* _MX6VAR_EEPROM_H_ */
