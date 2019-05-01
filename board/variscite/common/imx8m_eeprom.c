/*
 * Copyright (C) 2018-2019 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <i2c.h>

#include "imx8m_eeprom.h"

#ifdef CONFIG_DM_I2C
static struct udevice *var_eeprom_init(void)
{
	int ret;
	struct udevice *bus, *dev;

	ret = uclass_get_device_by_seq(UCLASS_I2C, VAR_EEPROM_I2C_BUS, &bus);
	if (ret) {
		debug("%s: No bus %d\n", __func__, VAR_EEPROM_I2C_BUS);
		return NULL;
	}

	ret = dm_i2c_probe(bus, VAR_EEPROM_I2C_ADDR, 0, &dev);
	if (ret) {
		debug("%s: Can't find device id=0x%x, on bus %d\n",
			__func__, VAR_EEPROM_I2C_ADDR, VAR_EEPROM_I2C_BUS);
		return NULL;
	}

	return dev;
}

int var_eeprom_read_header(struct var_eeprom *e)
{
	int ret;
	struct udevice *edev;

	edev = var_eeprom_init();
	if (!edev)
		return -1;

	/* Read EEPROM to memory */
	ret = dm_i2c_read(edev, 0, (void *)e, sizeof(*e));
	if (ret) {
		debug("EEPROM read failed, ret=%d\n", ret);
		return ret;
	}

	return 0;
}
#else
int var_eeprom_read_header(struct var_eeprom *e)
{
	int ret;

	/* Probe EEPROM */
	i2c_set_bus_num(VAR_EEPROM_I2C_BUS);
	ret = i2c_probe(VAR_EEPROM_I2C_ADDR);
        if (ret) {
		printf("EEPROM init failed\n");
		return -1;
        }

	/* Read EEPROM header to memory */
	ret = i2c_read(VAR_EEPROM_I2C_ADDR, 0, 1, (uint8_t *)e, sizeof(*e));
	if (ret) {
		printf("EEPROM read failed ret=%d\n", ret);
		return -1;
	}

	return 0;
}
#endif

int var_eeprom_get_mac(struct var_eeprom *e, u8 *buf)
{
	if (!var_eeprom_is_valid(e))
		return -1;

	memcpy(buf, e->mac, sizeof(e->mac));

	return 0;
}

int var_eeprom_get_dram_size(struct var_eeprom *e, u32 *size)
{
	u8 dramsize;

	if (!var_eeprom_is_valid(e))
		return -1;

	memcpy(&dramsize, (void *)&e->dramsize, sizeof(e->dramsize));

	if (e->version == 1)
		*size = dramsize * 1024;
	else
		*size = dramsize * 128;

	return 0;
}

void var_eeprom_print_prod_info(struct var_eeprom *e)
{
	if (!var_eeprom_is_valid(e))
		return;

#ifdef CONFIG_TARGET_IMX8M_VAR_DART
	printf("\nPart number: VSM-DT8M-%.*s\n", (int)sizeof(e->partnum), (char *)e->partnum);
#else
	printf("\nPart number: VSM-DT8MM-%.*s\n", (int)sizeof(e->partnum), (char *)e->partnum);
#endif
	printf("Assembly: AS%.*s\n", (int)sizeof(e->assembly), (char *)e->assembly);

	printf("Production date: %.*s %.*s %.*s\n",
			4, /* YYYY */
			(char *)e->date,
			3, /* MMM */
			((char *)e->date) + 4,
			2, /* DD */
			((char *)e->date) + 4 + 3);

	printf("Serial Number: %02x:%02x:%02x:%02x:%02x:%02x\n",
		e->mac[0], e->mac[1], e->mac[2], e->mac[3], e->mac[4], e->mac[5]);

	debug("EEPROM version: 0x%x\n", e->version);
	debug("SOM features: 0x%x\n", e->features);
	debug("DRAM size: %d GiB\n\n", e->dramsize);
}
