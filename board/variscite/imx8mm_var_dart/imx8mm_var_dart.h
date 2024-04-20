/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018 NXP
 * Copyright 2018-2023 Variscite Ltd.
 */

int var_get_som_rev(struct var_eeprom *ep);
int get_board_id(void);

enum {
	DART_MX8M_MINI,
	VAR_SOM_MX8M_MINI,
	UNKNOWN_BOARD,
};
