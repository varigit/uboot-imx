/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019-2025 Variscite Ltd.
 *
 */

int var_get_board_id(struct var_eeprom *ep);

enum {
        SPEAR_MX8,
        VAR_SOM_MX8,
        UNKNOWN_REV
};

/* Carrier board EEPROM */
#define CARRIER_EEPROM_BUS_SOM		0x04
#define CARRIER_EEPROM_BUS_SPEAR	0x00
#define CARRIER_EEPROM_ADDR		0x54
