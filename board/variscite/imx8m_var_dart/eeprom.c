/*
 * Copyright (C) 2018 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <i2c.h>

#include "eeprom.h"

static struct var_eeprom e;
static int eeprom_is_valid = 0;
static int eeprom_was_read = 0;
static struct udevice *edev = NULL;

/**
 * read_eeprom - read the EEPROM into memory
 */
static int var_eeprom_init(void)
{
	int ret;
	int i2c_bus = VAR_EEPROM_I2C_BUS;
	uint8_t chip = VAR_EEPROM_I2C_ADDR;
	struct udevice *bus, *dev;

	if (edev)
		return 0;

	ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
	if (ret) {
		eeprom_debug("%s: No bus %d\n", __func__, i2c_bus);
		return ret;
	}

	ret = dm_i2c_probe(bus, chip, 0, &dev);
	if (ret) {
		eeprom_debug("%s: Can't find device id=0x%x, on bus %d\n",
				__func__, chip, i2c_bus);
		return ret;
	}

	edev = dev;

	return 0;
}

/**
 * read_eeprom - read the EEPROM into memory
 */
static int var_read_eeprom(void)
{
	int ret;

	/* EEPROM was read to memory and validated */
	if (eeprom_is_valid)
		return 0;

	/* EEPROM was read to memory and validation failed */
	if (eeprom_was_read)
		return -1;

	/* Initialize EEPROM */
	ret = var_eeprom_init();
	if (ret)
		return ret;

	/* Read EEPROM to memory */
	ret = dm_i2c_read(edev, 0, (void *)&e, sizeof(e));
	if (ret)
		return ret;

	/* Mark EEPROM as read */
	eeprom_was_read = 1;

	if (htons(e.magic) != VAR_EEPROM_MAGIC) {
		eeprom_debug("Invalid EEPROM magic 0x%hx, expected 0x%hx\n",
			htons(e.magic), VAR_EEPROM_MAGIC);
		return -1;
	}

	/* Mark EEPROM as valid */
	eeprom_is_valid = 1;

	return 0;
}

int var_eeprom_read_mac(u8 *buf)
{
	int ret;

	ret = var_read_eeprom();
	if (ret)
		return ret;

	memcpy(buf, e.mac, sizeof(e.mac));

	return 0;
}

int var_eeprom_read_dram_size(u8 *buf)
{
	int ret;

	ret = var_read_eeprom();
	if (ret)
		return ret;

	memcpy(buf, (void* )&e.rs, sizeof(e.rs));

	return 0;
}

void var_eeprom_print_info(void)
{
	int ret;

	ret = var_read_eeprom();
	if (ret)
		return;

	printf("\nPart number: VSM-DT8M-%.*s\n", (int)sizeof(e.pn), (char *)e.pn);

	printf("Assembly: AS%.*s\n", (int)sizeof(e.as), (char *)e.as);

	printf("Production date: %.*s %.*s %.*s\n",
			4, /* YYYY */
			(char *)e.date,
			3, /* MMM */
			((char *)e.date) + 4,
			2, /* DD */
			((char *)e.date) + 4 + 3);

	printf("Serial Number: %02x:%02x:%02x:%02x:%02x:%02x\n",
		e.mac[0], e.mac[1], e.mac[2], e.mac[3], e.mac[4], e.mac[5]);

	switch (e.sr) {
	case 0:
		printf("SOM revision: 1.1\n\n");
		break;
	default:
		printf("SOM revision: unknown\n\n");
	}

#ifdef EEPROM_DEBUG
	printf("EEPROM version: 0x%x\n", e.ver);
	printf("SOM options: 0x%x\n", e.opt);
	printf("DRAM size: %dGiB\n\n", e.rs);
#endif
}


