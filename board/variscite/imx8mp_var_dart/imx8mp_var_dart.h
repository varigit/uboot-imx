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

/* Carrier board EEPROM */
#define CARRIER_EEPROM_BUS_SOM		0x03
#define CARRIER_EEPROM_BUS_DART		0x01
#define CARRIER_EEPROM_ADDR		0x54

int var_detect_board_id(void);
int var_detect_dart_carrier_rev(void);

#endif
