// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 *
 * Setup DRAM parameters and calibration values
 * for the specific DRAM on the board.
 * The parameters were provided by
 * i.MX6 DDR Stress Test Tool and saved on EEPROM.
 */

#include <command.h>
#include <i2c_eeprom.h>
#include <i2c.h>
#include <stdlib.h>
#include <asm/global_data.h>
#include <dm/uclass.h>
#include <linux/delay.h>

#include "mx6var_eeprom_v2.h"

DECLARE_GLOBAL_DATA_PTR;

void var_eeprom_print_prod_infos(struct var_eeprom *e)
{
	if (!var_eeprom_is_valid(e))
		return;

	/* Make sure strings are null terminated. */
	e->partnumber[DART6UL_PN_LEN - 1] = '\0';
	e->assy[DART6UL_ASSY_LEN - 1] = '\0';
	e->date[DART6UL_DATE_LEN - 1] = '\0';

	printf("Board: PN: %s, Assy: %s, Date: %s\n"
		"       Storage: %s, Wifi: %s, DDR: %d MiB, Rev: %s\n",
		e->partnumber, e->assy, e->date,
		som_info_storage_to_str(e->som_info),
		DART6UL_INFO_WIFI_GET(e->som_info) ? "yes" : "no",
		DART6UL_DDRSIZE(e->ddr_size) / SZ_1M,
		som_info_rev_to_str(e->som_info));
}

const char *som_info_rev_to_str(u8 som_info)
{
	switch (DART6UL_INFO_REV_GET(som_info)) {
	case 0x0: return "2.4G LWB";
	case 0x1: return "5G LW5";
	case 0x2: return "5G IW611";
	case 0x3: return "5G IW612";
	default: return "unknown";
	}
}

const char *som_info_storage_to_str(u8 som_info)
{
	switch (DART6UL_INFO_STORAGE_GET(som_info)) {
	case 0x0: return "none";
	case 0x1: return "nand";
	case 0x2: return "emmc";
	default: return "unknown";
	}
}

#if IS_ENABLED(CONFIG_SPL_BUILD)
int var_eeprom_read_header(struct var_eeprom *e)
{
	int ret;

	/* Probe EEPROM */
	i2c_set_bus_num(VAR_DART_EEPROM_I2C_BUS);
	ret = i2c_probe(VAR_DART_EEPROM_I2C_ADDR);
	if (ret) {
		printf("%s: I2C EEPROM probe failed\n", __func__);
		return ret;
	}

	/* Read EEPROM header to memory */
	ret = i2c_read(VAR_DART_EEPROM_I2C_ADDR, 0, 1, (uint8_t *)e, sizeof(*e));
	if (ret) {
		printf("%s: EEPROM read failed ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

/*
 * Fills custom_addresses & custom_values, from custom_addresses_values
 */
static void load_custom_data(u32 *custom_addresses, u32 *custom_values, const u32 *custom_addr_val)
{
	int i, j = 0;

	for (i = 0; i < MAX_CUSTOM_ADDRESSES; i++) {
		if (custom_addr_val[i] == 0)
			break;
		custom_addresses[i] = custom_addr_val[i];
	}

	i++;
	if (i > MAX_CUSTOM_ADDRESSES)
		return;

	j = 0;
	for (; i < MAX_CUSTOM_VALUES; i++) {
		if (custom_addr_val[i] == 0)
			break;
		custom_values[j] = custom_addr_val[i];
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

static int handle_commands(const struct cmd commands[],
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
				commands[i].addr,
				commands[i].index);

		if (commands[i].addr == LAST_COMMAND_INDEX)
			return 0;

		if (commands[i].index == DELAY_10USEC_INDEX) {
			/* Delay for Value * 10 uSeconds */
			eeprom_v2_debug("Delaying for %d microseconds\n", commands[i].index * 10);
			udelay((int)(commands[i].index * 10));
			++i;
			continue;
		}

		/*
		 * Check for a wait index.
		 * A wait index means "next command is a wait command".
		 */
		switch (commands[i].addr) {
		case WHILE_NOT_EQUAL_INDEX:
		case WHILE_EQUAL_INDEX:
		case WHILE_AND_INDEX:
		case WHILE_NOT_AND_INDEX:
			/* Save wait index and go to next command */
			wait_idx = commands[i].addr;
			++i;
			break;
		}

		/* Get address and value */
		address = get_address_by_index(commands[i].addr,
					       common_addresses,
					       custom_addresses);
		value = get_value_by_index(commands[i].index, common_values, custom_values);
		reg_ptr = (u32 *)address;

		switch (wait_idx) {
		case WHILE_NOT_EQUAL_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x is not equal %08x\n",
					address, value);
			while (*reg_ptr != value);
			break;
		case WHILE_EQUAL_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x is equal %08x\n",
					address, value);
			while (*reg_ptr == value);
			break;
		case WHILE_AND_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x and %08x is not zero\n",
					address, value);
			while (*reg_ptr & value);
			break;
		case WHILE_NOT_AND_INDEX:
			eeprom_v2_debug("Waiting while data at address %08x and %08x is zero\n",
					address, value);
			while (!(*reg_ptr & value));
			break;
		default:
			if (address == 0x021B0020 && value == 0x00007800)
				value = 0x00000800;

			/* This is a regular set command (non-wait) */
			eeprom_v2_debug("Setting data at address %08x to %08x\n",
					address, value);
			*reg_ptr = value;
			break;
		}

		wait_idx = 0;
		++i;
	}

	return 0;
}

int var_eeprom_dram_init(struct var_eeprom *e)
{
	if (!var_eeprom_is_valid(e))
		return -EINVAL;
	/*
	 * The eeprom contains commands with
	 * 1 byte index to a common address in this array, and
	 * 1 byte index to a common value in the next array - to write to the address.
	 */
	const u32 common_addresses[] = {
		#include "addresses.inc"
	};

	const u32 common_values[] = {
		#include "values.inc"
	};

	/*
	 * Some commands in the eeprom contain higher indices,
	 * to custom addresses and values which are not present in the common arrays,
	 * and it also contains an array of the custom addresses and values themselves.
	 */
	u32 custom_addresses[MAX_CUSTOM_ADDRESSES] = {0};
	u32 custom_values[MAX_CUSTOM_VALUES] = {0};

	load_custom_data(custom_addresses, custom_values, e->custom_addr_val);

	return handle_commands(e->custom_cmd, common_addresses, common_values,
				custom_addresses, custom_values);
}
#endif /* CONFIG_SPL_BUILD */
