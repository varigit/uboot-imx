#ifndef __IMX8MP_VAR_DART_H__
#define __IMX8MP_VAR_DART_H__

enum {
	BOARD_ID_SOM,
	BOARD_ID_DART,
	BOARD_ID_UNDEF,
};

int var_detect_board_id(void);

/* Carrier board EEPROM */
#define CARRIER_EEPROM_BUS_SOM		0x03
#define CARRIER_EEPROM_BUS_DART		0x01
#define CARRIER_EEPROM_ADDR		0x54

#endif
