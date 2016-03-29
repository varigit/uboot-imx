/*
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _MX6VAR_LEGACY_H_
#define _MX6VAR_LEGACY_H_

void reset_ddr_solo(void);
void spl_dram_init_mx6solo_512mb(void);
void spl_mx6qd_dram_setup_iomux(void);
void spl_dram_init_mx6dl_1g(void);
void spl_dram_init_mx6q_1g(void);
void spl_dram_init_mx6q_2g(void);
void spl_mx6dlsl_dram_setup_iomux(void);
void spl_dram_init_mx6solo_1gb(void);
u32 get_actual_ram_size(u32 max_size);

#endif /* _MX6VAR_LEGACY_H_ */
