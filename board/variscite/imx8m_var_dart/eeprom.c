/*
 * Copyright (C) 2018 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <i2c.h>

#include "eeprom.h"

static struct udevice *eeprom_dev = NULL;

static int var_eeprom_init(void)
{
	int ret;
	u16 magic;
	int i2c_bus = VAR_EEPROM_I2C_BUS;
	uint8_t chip = VAR_EEPROM_I2C_ADDR;
	struct udevice *bus, *dev;
	u32 offset = offsetof(struct var_eeprom, magic);

	if (eeprom_dev)
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

	ret = dm_i2c_read(dev, offset, (u8 *)&magic, sizeof(magic));
	if (ret) {
		eeprom_debug("I2C read failed for bus %d addr %d\n",
			VAR_EEPROM_I2C_BUS, VAR_EEPROM_I2C_ADDR);
		return ret;
	}

	if (magic != VAR_EEPROM_MAGIC) {
		eeprom_debug("Invalid EEPROM magic 0x%x, expected 0x%x",
			magic, VAR_EEPROM_MAGIC);
		return -1;
	}

	eeprom_dev = dev;

	return 0;
}


static int var_eeprom_read(u32 offset, u8 *buf, int len)
{
	int ret;

	ret = var_eeprom_init();
	if (ret)
		return ret;

	return dm_i2c_read(eeprom_dev, offset, buf, len);
}

int var_eeprom_read_mac(u8 *buf)
{
	u32 offset = offsetof(struct var_eeprom, mac);

	return var_eeprom_read(offset, buf, 6);
}

int var_eeprom_read_dram_size(u8 *buf)
{
	u32 offset = offsetof(struct var_eeprom, dram_size);

	return var_eeprom_read(offset, buf, 1);
}


