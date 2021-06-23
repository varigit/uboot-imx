/*
 * Copyright 2018-2021 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IMX8MM_VAR_DART_H__
#define __IMX8MM_VAR_DART_H__

/* Board ID */
enum {
	DART_MX8M_MINI,
	VAR_SOM_MX8M_MINI,
	UNKNOWN_BOARD,
};

/* VAR-SOM-IMX8M-MINI revision */
enum {
	SOM_REV_10,
	SOM_REV_11,
	SOM_REV_12,
	SOM_REV_13,
	UNKNOWN_REV
};

/* DT8MCarrierBoard revision */
enum {
	DART_CARRIER_REV_1,
	DART_CARRIER_REV_2,
	DART_CARRIER_REV_UNDEF,
};

int var_get_board_id(void);
int var_get_som_rev(struct var_eeprom *ep);

#endif
