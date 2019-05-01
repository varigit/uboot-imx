/*
 * Copyright (C) 2018-2019 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <i2c.h>
#include <asm/arch/imx8m_ddr.h>

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

#ifdef CONFIG_SPL_BUILD
/*
 * Modify DRAM table based on adjustment table in EEPROM
 *
 * Assumption: register addresses in the adjustment table
 * follow the order of register addresses in the original table
 *
 * @adj_table_offset - offset of adjustment table from start of EEPROM
 * @adj_table_size   - number of rows in adjustment table
 * @table            - pointer to DDR table
 * @table_size       - number of rows in DDR table
 */
static void adjust_dram_table(u8 adj_table_offset, u8 adj_table_size,
				struct dram_cfg_param *table, u8 table_size)
{
	int i, j = 0;
	u8 off = adj_table_offset;
	struct dram_cfg_param adj_table_row;

	/* Iterate over adjustment table */
	for (i = 0; i < adj_table_size; i++) {

		/* Read next entry from adjustment table */
		i2c_read(VAR_EEPROM_I2C_ADDR, off, 1,
			(uint8_t *)&adj_table_row, sizeof(adj_table_row));

		/* Iterate over DDR table and adjust it */
		for (; j < table_size; j++) {
			if (table[j].reg == adj_table_row.reg) {
				debug("Adjusting reg=0x%x val=0x%x\n",
					adj_table_row.reg, adj_table_row.val);
				table[j].val = adj_table_row.val;
				break;
			}
		}

		off += sizeof(adj_table_row);
	}
}

/*
 * Modify DRAM tables based on adjustment tables in EEPROM
 *
 * @e - pointer to EEPROM header structure
 * @d - pointer to DRAM configuration structure
  */
void var_eeprom_adjust_dram(struct var_eeprom *e, struct dram_timing_info *d)
{
	int i;
	int *idx_map;
	u8 adj_table_size[DRAM_TABLE_NUM];

	/* Indices of fsp tables in the offset table */
	int b0_idx_map[] = {3, 4, 6};
	int b1_idx_map[] = {3, 4, 5, 6};

	/* Check EEPROM validity */
	if (!var_eeprom_is_valid(e))
		return;

	/* Check EEPROM version - only version 2+ has DDR adjustment tables */
	if (e->version < 2) {
		debug("EEPROM version is %d\n", e->version);
		return;
	}

	debug("EEPROM offset table\n");
	for (i = 0; i < DRAM_TABLE_NUM + 1; i++)
		debug("off[%d]=%d\n", i, e->off[i]);

	/* Calculate DRAM adjustment table sizes */
	for (i = 0; i < DRAM_TABLE_NUM; i++)
		adj_table_size[i] = (e->off[i + 1] - e->off[i]) /
				(sizeof(struct dram_cfg_param));

	debug("\nSizes table\n");
	for (i = 0; i < DRAM_TABLE_NUM; i++)
		debug("sizes[%d]=%d\n", i, adj_table_size[i]);

	/* Adjust DRAM controller configuration table */
	debug("\nAdjusting DDRC table: offset=%d, count=%d\n",
		e->off[0], adj_table_size[0]);
	adjust_dram_table(e->off[0], adj_table_size[0],
				d->ddrc_cfg, d->ddrc_cfg_num);

	/* Adjust DDR PHY configuration table */
	debug("\nAdjusting DDR PHY CFG table: offset=%d, count=%d\n",
		e->off[1], adj_table_size[1]);
	adjust_dram_table(e->off[1], adj_table_size[1],
				d->ddrphy_cfg, d->ddrphy_cfg_num);

	/* Adjust DDR PHY PIE table */
	debug("\nAdjusting DDR PHY PIE table: offset=%d, count=%d\n",
		e->off[2], adj_table_size[2]);
	adjust_dram_table(e->off[2], adj_table_size[2],
				d->ddrphy_pie, d->ddrphy_pie_num);

	/* Adjust FSP configuration tables
	 * i.MX8M B0 has 3 tables, i.MX8M B1 and i.MX8M Mini have 4 tables
	 */
	idx_map = (d->fsp_msg_num == 4) ? b1_idx_map : b0_idx_map;
	for (i = 0; i < d->fsp_msg_num; i++) {
		int j = idx_map[i];
		debug("\nAdjusting FSP table %d: offset=%d, count=%d\n",
			i, e->off[j], adj_table_size[j]);
		adjust_dram_table(e->off[j], adj_table_size[j],
			d->fsp_msg[i].fsp_cfg, d->fsp_msg[i].fsp_cfg_num);
	}
}
#endif
