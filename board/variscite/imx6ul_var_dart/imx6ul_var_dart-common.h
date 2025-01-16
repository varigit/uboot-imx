/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2015-2025 Variscite Ltd.
 */

#ifndef __IMX6UL_VAR_DART_COMMON_H
#define __IMX6UL_VAR_DART_COMMON_H

#include <i2c.h>
#include <hang.h>

#endif /* __DART_6UL_COMMON_H */

#define GPIO_SOM_ID	IMX_GPIO_NR(4, 13)

#if IS_ENABLED(CONFIG_NAND_MXS)
#define GPMI_PAD_CTRL0 (PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_100K_UP)
#define GPMI_PAD_CTRL1 (PAD_CTL_DSE_40ohm | PAD_CTL_SPEED_MED | \
			PAD_CTL_SRE_FAST)
#define GPMI_PAD_CTRL2 (GPMI_PAD_CTRL0 | GPMI_PAD_CTRL1)
#endif

/*
 * Returns true if the SOM is DART-6UL
 */
static inline bool is_dart_6ul(void)
{
	static int is_dart = -1;

	if (is_dart == -1) {
		imx_iomux_v3_setup_pad(MX6_PAD_NAND_CE0_B__GPIO4_IO13 |
				MUX_PAD_CTRL(PAD_CTL_PUS_100K_UP));

		gpio_request(GPIO_SOM_ID, "SOM ID");
		gpio_direction_input(GPIO_SOM_ID);
		is_dart = (gpio_get_value(GPIO_SOM_ID) != 0);

#if IS_ENABLED(CONFIG_NAND_MXS)
		imx_iomux_v3_setup_pad(MX6_PAD_NAND_CE0_B__RAWNAND_CE0_B |
				MUX_PAD_CTRL(GPMI_PAD_CTRL2));
#endif
	}
	return is_dart;
}

static inline bool is_var_som_6ul(void)
{
	return !is_dart_6ul();
}

#define DART_6UL_IDX 0
#define VAR_SOM_6UL_IDX 1

static inline int get_board_indx(void)
{
	if (is_dart_6ul())
		return DART_6UL_IDX;
	if (is_var_som_6ul())
		return VAR_SOM_6UL_IDX;

	printf("Error identifying board!\n");
	hang();
	return -1;
}

#define GPIO_EXPANDER_I2C_I2C_BUS	0
#define GPIO_EXPANDER_I2C_ADDR 0x20

#if IS_ENABLED(CONFIG_SPL_BUILD)
static inline bool is_symphony(void)
{
	/* Locate the I2C bus */
	i2c_set_bus_num(GPIO_EXPANDER_I2C_I2C_BUS);
	if (i2c_probe(GPIO_EXPANDER_I2C_ADDR))
		return false;

	return true;
}
#endif /* IMX6UL_VAR_DART_COMMON_H */
