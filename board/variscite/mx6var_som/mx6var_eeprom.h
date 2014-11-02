/*
 * Copyright (C) 2014 Variscite Ltd. All Rights Reserved.
 * Maintainer: Ron Donio <ron.d@variscite.com>
 * Configuration settings for the Variscite VAR_SOM_MX6 board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#if !defined(VAR_EEPROM_IMX6_H)
#define VAR_EEPROM_IMX6_H

#define VARISCITE_MAGIC 0x49524157 // == HEX("VARI")

#define VARISCITE_MX6_EEPROM_CHIP 0x56

#define VARISCITE_MX6_EEPROM_STRUCT_OFFSET 0x00000000

#define VARISCITE_MX6_EEPROM_WRITE_MAX_SIZE 0x4

#define EEPROM_SIZE_BYTES 512

#define VAR_DDR_INIT_OPCODE_WRITE_VAL_TO_ADDRESS	0x00000000
#define VAR_DDR_INIT_OPCODE_WAIT_MASK_RISING		0x10000000
#define VAR_DDR_INIT_OPCODE_WAIT_MASK_FALLING		0x20000000
#define VAR_DDR_INIT_OPCODE_DELAY_USEC			0x30000000

#define SPL_DRAM_INIT_STATUS_GEN_ERROR				-1
#define	SPL_DRAM_INIT_STATUS_OK					0
#define	SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM			1
#define	SPL_DRAM_INIT_STATUS_ERROR_NO_EEPROM_STRUCT_DETECTED	2

typedef struct {
	u32 variscite_magic; // == HEX("VARI")?
	u8 part_number[16];
	u8 Assembly[16];
	u8 date[16];
	u8 version;
	u8 reserved[7];
	u32 ddr_size;
} var_eeprom_config_struct_header_t;

#define IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE_ADDR	0x020e0774
#define IOMUXC_SW_PAD_CTL_GRP_DDRPKE_ADDR	0x020e0754
#define IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL_ADDR	0x020e0750
#define IOMUXC_SW_PAD_CTL_GRP_DDRMODE_ADDR	0x020e0760
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2_ADDR	0x020e04a0

#define IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS_ADDR 0x020e0464
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS_ADDR 0x020e0490
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1_ADDR 0x020e04c0
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2_ADDR 0x020e04c4
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3_ADDR 0x020e04c8

typedef struct {
	u32 IOMUXC_SW_PAD_CTL_GRP_DDR_TYPE;	// address = 0x020e0774
	u32 IOMUXC_SW_PAD_CTL_GRP_DDRPKE;	// address = 0x020e0754
	u32 IOMUXC_SW_PAD_CTL_GRP_DDRMODE_CTL;	// address = 0x020e0750
	u32 IOMUXC_SW_PAD_CTL_GRP_DDRMODE;	// address = 0x020e0760
	u32 IOMUXC_SW_PAD_CTL_PAD_DRAM_SDBA2;	// address = 0x020e04a0
	u32 dram_reset;
	u32 dram_sdcke0;
	u32 dram_sdcke1;
	u32 dram_sdodt0;
	u32 dram_sdodt1;
	u32 reserved;
	u32 pinmux_ctrlpad_all_value;
} var_pinmux_group_regs_t;

typedef struct {
        u32 address; // address encoded with opcode
        u32 value; // value_or_mask;
} reg_write_opcode_t;

typedef struct {
	var_eeprom_config_struct_header_t header;
	var_pinmux_group_regs_t pinmux_group;
	reg_write_opcode_t write_opcodes[ (EEPROM_SIZE_BYTES - sizeof(var_eeprom_config_struct_header_t) - sizeof(var_pinmux_group_regs_t)) / sizeof(reg_write_opcode_t) ];
} var_eeprom_config_struct_t;

bool var_eeprom_is_valid(var_eeprom_config_struct_t *p_var_eeprom_cfg);

int var_eeprom_write_struct(var_eeprom_config_struct_t *p_var_eeprom_cfg);

// init ddr iomux from struct
void var_eeprom_mx6dlsl_dram_setup_iomux_from_struct(var_pinmux_group_regs_t *pinmux_group);

void var_eeprom_mx6qd_dram_setup_iomux_from_struct(var_pinmux_group_regs_t *pinmux_group);

// init ddr from struct
void var_eeprom_dram_init_from_struct(var_eeprom_config_struct_t *p_var_eeprom_cfg);

int var_eeprom_i2c_init(void);

int var_eeprom_read_struct(var_eeprom_config_struct_t *p_var_eeprom_cfg);

void var_eeprom_strings_print(var_eeprom_config_struct_t *p_var_eeprom_cfg);

void var_eeprom_printf_array_dwords(u32 *ptr, u32 address_mem, u32 size);

#endif/* VAR_EEPROM_IMX6_H */
