/*
 * Copyright (C) 2015-2017 Variscite Ltd.
 *
 * Setup DRAM parametes and calibration values
 * for the specific DRAM on the board.
 * The parameters were provided by
 * i.MX6 DDR Stress Test Tool and saved on EEPROM.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <i2c.h>
#include "mx6var_eeprom_v2.h"

static inline bool var_eeprom_v2_is_valid(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	return (VARISCITE_MAGIC_V2 == p_var_eeprom_v2_cfg->variscite_magic);
}

int var_eeprom_v2_read_struct(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	i2c_set_bus_num(VAR_DART_EEPROM_I2C_BUS);
	if (i2c_probe(VAR_DART_EEPROM_I2C_ADDR)) {
		eeprom_v2_debug("\nError: Couldn't find EEPROM device\n");
		return -1;
	}

	if (i2c_read(VAR_DART_EEPROM_I2C_ADDR, 0, 1,
				(u8 *) p_var_eeprom_v2_cfg,
				sizeof(struct var_eeprom_v2_cfg))) {
		eeprom_v2_debug("\nError reading data from EEPROM\n");
		return -1;
	}

	if (!var_eeprom_v2_is_valid(p_var_eeprom_v2_cfg)) {
		eeprom_v2_debug("\nError: Data on EEPROM is invalid\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_SPL_BUILD
/*
 * Fills custom_addresses & custom_values, from custom_addresses_values
 */
static void load_custom_data(u32 *custom_addresses, u32 *custom_values, const u32 *custom_addresses_values)
{
	int i, j=0;

	for (i = 0; i < MAX_CUSTOM_ADDRESSES; i++) {
		if (custom_addresses_values[i] == 0)
			break;
		custom_addresses[i] = custom_addresses_values[i];
	}

	i++;
	if (i > MAX_CUSTOM_ADDRESSES)
		return;

	j=0;
	for (; i < MAX_CUSTOM_VALUES; i++) {
		if (custom_addresses_values[i] == 0)
			break;
		custom_values[j] = custom_addresses_values[i];
		j++;
	}
}

static u32 get_address_by_index(u8 index, const u32 *common_addresses, const u32 *custom_addresses)
{
	if (index >= MAX_COMMON_ADDRS_INDEX)
		return custom_addresses[index - MAX_COMMON_ADDRS_INDEX];

	return common_addresses[index];
}

static u32 get_value_by_index(u8 index, const u32 *common_values, const u32 *custom_values)
{
	if (index >= MAX_COMMON_VALUES_INDEX)
		return custom_values[index - MAX_COMMON_VALUES_INDEX];

	return common_values[index];
}

static int handle_commands(const struct eeprom_command commands[],
			const u32 *common_addresses,
			const u32 *common_values,
			const u32 *custom_addresses,
			const u32 *custom_values)
{
	u32 address, value;
	volatile u32 *reg_ptr;
	u8 wait_idx = 0;
	int i = 0;

	while (i < MAX_NUM_OF_COMMANDS) {

		eeprom_v2_debug("Executing command %03d: %03d,  %03d\n",
				i,
				commands[i].address_index,
				commands[i].value_index);

		if (commands[i].address_index == LAST_COMMAND_INDEX)
			return 0;

		if (commands[i].address_index == DELAY_10USEC_INDEX) {
			/* Delay for Value * 10 uSeconds */
			eeprom_v2_debug("Delaying for %d microseconds\n", commands[i].value_index * 10);
			udelay((int)(commands[i].value_index * 10));
			++i;
			continue;
		}

		/*
		 * Check for a wait index.
		 * A wait index means "next command is a wait command".
		 */
		switch (commands[i].address_index) {
		case WHILE_NOT_EQUAL_INDEX:
		case WHILE_EQUAL_INDEX:
		case WHILE_AND_INDEX:
		case WHILE_NOT_AND_INDEX:
			/* Save wait index and go to next command */
			wait_idx = commands[i].address_index;
			++i;
			break;
		}

		/* Get address and value */
		address = get_address_by_index(commands[i].address_index, common_addresses, custom_addresses);
		value = get_value_by_index(commands[i].value_index, common_values, custom_values);
		reg_ptr = (u32 *)address;

		switch (wait_idx) {
		case WHILE_NOT_EQUAL_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x is not equal %08x\n", address, value);
			while(*reg_ptr != value);
			break;
		case WHILE_EQUAL_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x is equal %08x\n", address, value);
			while(*reg_ptr == value);
			break;
		case WHILE_AND_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x and %08x is not zero\n", address, value);
			while(*reg_ptr & value);
			break;
		case WHILE_NOT_AND_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x and %08x is zero\n", address, value);
			while(!(*reg_ptr & value));
			break;
		default:
			/* This is a regular set command (non-wait) */
			eeprom_v2_debug("Setting data at address %08x to %08x\n", address, value);
			*reg_ptr = value;
			break;
		}

		++i;
	}

	return 0;
}

int var_eeprom_v2_dram_init(struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	/*
	 * The eeprom contains commands with
	 * 1 byte index to a common address in this array, and
	 * 1 byte index to a common value in the next array - to write to the address.
	 */
	const u32 common_addresses[]=
	{
		#include "addresses.inc"
	};

	const u32 common_values[] =
	{
		#include "values.inc"
	};

	/*
	 * Some commands in the eeprom contain higher indices,
	 * to custom addresses and values which are not present in the common arrays,
	 * and it also contains an array of the custom addresses and values themselves.
	 */
	u32 custom_addresses[MAX_CUSTOM_ADDRESSES]={0};
	u32 custom_values[MAX_CUSTOM_VALUES]={0};

	load_custom_data(custom_addresses, custom_values,
			p_var_eeprom_v2_cfg->custom_addresses_values);

	return handle_commands(p_var_eeprom_v2_cfg->commands,
			       common_addresses, common_values,
			       custom_addresses, custom_values);
}

void var_eeprom_v2_print_production_info(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	printf("\nPart number: %.*s\n",
			sizeof(p_var_eeprom_v2_cfg->part_number) - 1,
			(char *) p_var_eeprom_v2_cfg->part_number);

	printf("Assembly: %.*s\n",
			sizeof(p_var_eeprom_v2_cfg->assembly) - 1,
			(char *) p_var_eeprom_v2_cfg->assembly);

	printf("Date of production: %.*s\n",
			sizeof(p_var_eeprom_v2_cfg->date) - 1,
			(char *) p_var_eeprom_v2_cfg->date);
}

void var_eeprom_v2_print_som_info(const struct var_eeprom_v2_cfg *p_var_eeprom_v2_cfg)
{
	puts("DART-6UL");
	if (((p_var_eeprom_v2_cfg->som_info >> 3) & 0x3) == 1)
		puts("-5G");
	puts(" configuration: ");
	switch(p_var_eeprom_v2_cfg->som_info & 0x3) {
	case 0x00:
		puts("SD card Only ");
		break;
	case 0x01:
		puts("NAND ");
		break;
	case 0x02:
		puts("eMMC ");
		break;
	case 0x03:
		puts("Ilegal !!! ");
		break;
	}
	if (p_var_eeprom_v2_cfg->som_info & 0x04)
		puts("WiFi");
	puts("\n");
}
#else
static int var_eeprom_write(uchar *ptr, u32 size, u32 eeprom_i2c_addr, u32 offset)
{
	int ret = 0;
	u32 size_written;
	u32 size_to_write;
	u32 P0_select_page_EEPROM;
	u32 chip;
	u32 addr;

	/* Write to EEPROM device */
	size_written = 0;
	size_to_write = size;
	while ((ret == 0) && (size_written < size_to_write)) {
		P0_select_page_EEPROM = (offset > 0xFF);
		chip = eeprom_i2c_addr + P0_select_page_EEPROM;
		addr = (offset & 0xFF);
		ret = i2c_write(chip, addr, 1, ptr, VAR_EEPROM_WRITE_MAX_SIZE);

		/* Wait for EEPROM write operation to complete (No ACK) */
		mdelay(11);

		size_written += VAR_EEPROM_WRITE_MAX_SIZE;
		offset += VAR_EEPROM_WRITE_MAX_SIZE;
		ptr += VAR_EEPROM_WRITE_MAX_SIZE;
	}

	return ret;
}

/*
 * var_eeprom_params command intepreter.
 */
static int do_var_eeprom_params(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct var_eeprom_v2_cfg var_eeprom_cfg;
	int offset;

	if (argc != 5)
		return -1;

	i2c_set_bus_num(VAR_DART_EEPROM_I2C_BUS);
	if (i2c_probe(VAR_DART_EEPROM_I2C_ADDR)) {
		printf("Error: couldn't find EEPROM device\n");
		return -1;
	}

	memset(&var_eeprom_cfg, 0, sizeof(var_eeprom_cfg));

	memcpy(var_eeprom_cfg.part_number, argv[1], sizeof(var_eeprom_cfg.part_number) - 1);
	memcpy(var_eeprom_cfg.assembly, argv[2], sizeof(var_eeprom_cfg.assembly) - 1);
	memcpy(var_eeprom_cfg.date, argv[3], sizeof(var_eeprom_cfg.date) - 1);

	var_eeprom_cfg.som_info = (u8) simple_strtoul(argv[4], NULL, 16);

	printf("Part number: %s\n", (char *)var_eeprom_cfg.part_number);
	printf("Assembly: %s\n", (char *)var_eeprom_cfg.assembly);
	printf("Date of production: %s\n", (char *)var_eeprom_cfg.date);

	printf("SOM Configuration: 0x%x: ",var_eeprom_cfg.som_info);
	switch (var_eeprom_cfg.som_info & 0x3) {
	case 0x00:
		puts("SD card only");
		break;
	case 0x01:
		puts("NAND");
		break;
	case 0x02:
		puts("eMMC");
		break;
	case 0x03:
		puts("Ilegal!");
		break;
	}

	if (var_eeprom_cfg.som_info & 0x04)
		puts(", WiFi");

	switch ((var_eeprom_cfg.som_info >> 3) & 0x3) {
	case 0x0:
		puts(", SOM Rev 1\n");
		break;
	case 0x1:
		puts(", SOM Rev 2 (5G)\n");
		break;
	default:
		puts(", SOM Rev ilegal!\n");
		break;
	}

	offset = (uchar *) var_eeprom_cfg.part_number - (uchar *) &var_eeprom_cfg;
	if (var_eeprom_write((uchar *) var_eeprom_cfg.part_number,
				sizeof(var_eeprom_cfg.part_number),
				VAR_DART_EEPROM_I2C_ADDR,
				offset))
		goto err;

	offset = (uchar *) var_eeprom_cfg.assembly - (uchar *) &var_eeprom_cfg;
	if (var_eeprom_write((uchar *) var_eeprom_cfg.assembly,
				sizeof(var_eeprom_cfg.assembly),
				VAR_DART_EEPROM_I2C_ADDR,
				offset))
		goto err;

	offset = (uchar *) var_eeprom_cfg.date - (uchar *) &var_eeprom_cfg;
	if (var_eeprom_write((uchar *) var_eeprom_cfg.date,
				sizeof(var_eeprom_cfg.date),
				VAR_DART_EEPROM_I2C_ADDR,
				offset))
		goto err;

	offset = (uchar *) &var_eeprom_cfg.som_info - (uchar *) &var_eeprom_cfg;
	if (var_eeprom_write((uchar *) &var_eeprom_cfg.som_info,
				sizeof(var_eeprom_cfg.som_info),
				VAR_DART_EEPROM_I2C_ADDR,
				offset))
		goto err;

	printf("EEPROM updated successfully\n");

	return 0;

err:
	printf("Error writing to EEPROM!\n");
	return -1;
}

U_BOOT_CMD(
	vareeprom,	5,	1,	do_var_eeprom_params,
	"For internal use only",
	"- Do not use"
);
#endif
