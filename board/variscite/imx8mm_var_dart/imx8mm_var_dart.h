int var_get_som_rev(struct var_eeprom *ep);
int get_board_id(void);

enum {
	DART_MX8M_MINI,
	VAR_SOM_MX8M_MINI,
	UNKNOWN_BOARD,
};

enum {
	SOM_REV_10,
	SOM_REV_11,
	SOM_REV_12,
	SOM_REV_13,
	UNKNOWN_REV
};

enum {
	DART_CARRIER_REV_1,
	DART_CARRIER_REV_2,
	DART_CARRIER_REV_UNDEF,
};
