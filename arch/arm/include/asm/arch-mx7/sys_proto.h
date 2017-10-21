/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

#include <asm/imx-common/regs-common.h>
#include "../arch-imx/cpu.h"

#define is_soc_rev(rev)		((int)((get_cpu_rev() & 0xFF) - rev))
u32 get_cpu_rev(void);

/* returns MXC_CPU_ value */
#define cpu_type(rev) (((rev) >> 12)&0xff)
#define soc_type(rev) (((rev) >> 12)&0xf0)

/* use with MXC_CPU_ constants */
#define get_cpu_type()	(cpu_type(get_cpu_rev()))
#define get_soc_type()	(soc_type(get_cpu_rev()))
#define is_cpu_type(cpu) (get_cpu_type() == cpu)
#define is_soc_type(soc) (get_soc_type() == soc)

#define is_mx6() (is_soc_type(MXC_SOC_MX6))
#define is_mx7() (is_soc_type(MXC_SOC_MX7))

const char *get_imx_type(u32 imxtype);
unsigned imx_ddr_size(void);
void set_wdog_reset(struct wdog_regs *wdog);

int arch_auxiliary_core_up(u32 core_id, u32 boot_private_data);
int arch_auxiliary_core_check_up(u32 core_id);

#define is_mx6dqp() (is_cpu_type(MXC_CPU_MX6QP) || is_cpu_type(MXC_CPU_MX6DP))
#define is_mx6dq() (is_cpu_type(MXC_CPU_MX6Q) || is_cpu_type(MXC_CPU_MX6D))
#define is_mx6sdl() (is_cpu_type(MXC_CPU_MX6SOLO) || is_cpu_type(MXC_CPU_MX6DL))
#define is_mx6sx() (is_cpu_type(MXC_CPU_MX6SX))
#define is_mx6sl() (is_cpu_type(MXC_CPU_MX6SL))
#define is_mx6ul() (is_cpu_type(MXC_CPU_MX6UL))
#define is_mx6ull() (is_cpu_type(MXC_CPU_MX6ULL))

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */

int fecmxc_initialize(bd_t *bis);
u32 get_ahb_clk(void);
u32 get_periph_clk(void);

int mxs_reset_block(struct mxs_register_32 *reg);
int mxs_wait_mask_set(struct mxs_register_32 *reg,
		       uint32_t mask,
		       unsigned int timeout);
int mxs_wait_mask_clr(struct mxs_register_32 *reg,
		       uint32_t mask,
		       unsigned int timeout);
#endif
