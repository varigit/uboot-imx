/*
 * Copyright 2019-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

int var_get_board_id(struct var_eeprom *ep);

enum {
        SPEAR_MX8,
        VAR_SOM_MX8,
        UNKNOWN_REV
};
