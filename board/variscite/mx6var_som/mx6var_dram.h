/*
 * Copyright (C) 2016 Variscite Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX6VAR_DRAM_H_
#define _MX6VAR_DRAM_H_

int var_eeprom_v1_dram_init(void);
void var_legacy_dram_init(int is_som_solo);

void var_eeprom_v2_dram_init(void);

#endif /* _MX6VAR_DRAM_H_ */
