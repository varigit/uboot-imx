#ifndef __IMX8MP_VAR_DART_H__
#define __IMX8MP_VAR_DART_H__

enum {
	BOARD_ID_SOM,
	BOARD_ID_DART,
	BOARD_ID_UNDEF,
};

enum {
	DART_CARRIER_REV_1,
	DART_CARRIER_REV_2,
	DART_CARRIER_REV_UNDEF,
};

int var_detect_board_id(void);
int var_detect_dart_carrier_rev(void);

#endif
