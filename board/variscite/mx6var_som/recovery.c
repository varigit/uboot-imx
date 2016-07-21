/*
 * Copyright (C) 2010-2014 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <recovery.h>

void setup_recovery_env(void)
{
	board_recovery_setup();
}

/* export to lib_arm/board.c */
void check_recovery_mode(void)
{
	if (check_recovery_cmd_file()) {
		puts("Fastboot: Recovery command file found!\n");
		setup_recovery_env();
	} else {
		puts("Fastboot: Normal\n");
	}
}
