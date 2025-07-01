// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 */

#include <command.h>
#include <i2c_eeprom.h>
#include <i2c.h>
#include <stdlib.h>
#include <asm/global_data.h>
#include <dm/uclass.h>
#include <linux/delay.h>

#include "mx7dvar_eeprom.h"

DECLARE_GLOBAL_DATA_PTR;

struct var_eeprom_print_info {
	const u8 *part_number;
	size_t part_number_len;
	const u8 *assembly;
	size_t assembly_len;
	const u8 *date_year;
	size_t year_len;
	const u8 *date_month;
	size_t month_len;
	const u8 *date_day;
	size_t day_len;
};

static void print_production_info_common(const struct var_eeprom_print_info *info)
{
	char part_buf[info->part_number_len + 1];
	char assy_buf[info->assembly_len + 1];
	char year_buf[info->year_len + 1];
	char month_buf[info->month_len + 1];
	char day_buf[info->day_len + 1];

	memcpy(part_buf, info->part_number, info->part_number_len);
	part_buf[info->part_number_len] = '\0';

	memcpy(assy_buf, info->assembly, info->assembly_len);
	assy_buf[info->assembly_len] = '\0';

	memcpy(year_buf, info->date_year, info->year_len);
	year_buf[info->year_len] = '\0';

	memcpy(month_buf, info->date_month, info->month_len);
	month_buf[info->month_len] = '\0';

	memcpy(day_buf, info->date_day, info->day_len);
	day_buf[info->day_len] = '\0';

	printf("Part number: VSM-MX7-%s, Assembly: AS%s, Date of production: %s %s %s\n",
	       part_buf, assy_buf, day_buf, month_buf, year_buf);
}

void mx7d_var_eeprom_print_legacy_production_info(const struct mx7d_var_eeprom *e)
{
	struct mx7d_var_legacy_eeprom *legacy = (struct mx7d_var_legacy_eeprom *)e;

	struct var_eeprom_print_info info = {
		.part_number = legacy->part_number,
		.part_number_len = sizeof(legacy->part_number),

		.assembly = legacy->assembly,
		.assembly_len = sizeof(legacy->assembly),

		.date_year = legacy->date_year,
		.year_len = sizeof(legacy->date_year),

		.date_month = legacy->date_month,
		.month_len = sizeof(legacy->date_month),

		.date_day = legacy->date_day,
		.day_len = sizeof(legacy->date_day)
	};

	print_production_info_common(&info);
}

void mx7d_var_eeprom_print_production_info(const struct mx7d_var_eeprom *e)
{
	if (!mx7d_var_eeprom_is_valid(e))
		return;

	struct var_eeprom_print_info info = {
		.part_number = e->part_number,
		.part_number_len = sizeof(e->part_number),

		.assembly = e->assembly,
		.assembly_len = sizeof(e->assembly),

		.date_year = e->date_year,
		.year_len = sizeof(e->date_year),

		.date_month = e->date_month,
		.month_len = sizeof(e->date_month),

		.date_day = e->date_day,
		.day_len = sizeof(e->date_day)
	};

	print_production_info_common(&info);
}

#if IS_ENABLED(CONFIG_SPL_BUILD)

int mx7d_var_eeprom_read_header(struct mx7d_var_eeprom *e)
{
	int ret;

	i2c_set_bus_num(EEPROM_I2C_BUS);
	ret = i2c_probe(EEPROM_I2C_ADDR);
	if (ret) {
		printf("%s: I2C EEPROM probe failed\n", __func__);
		return ret;
	}

	/* Read EEPROM header to memory */
	ret = i2c_read(EEPROM_I2C_ADDR, 0, 1, (u8 *)e, sizeof(*e));
	if (ret) {
		printf("%s: EEPROM read failed ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

#endif /* CONFIG_SPL_BUILD */
