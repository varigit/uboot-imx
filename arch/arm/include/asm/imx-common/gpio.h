/*
 * Copyright (C) 2011
 * Stefano Babic, DENX Software Engineering, <sbabic@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#ifndef __ASM_ARCH_IMX_GPIO_H
#define __ASM_ARCH_IMX_GPIO_H

#if !(defined(__KERNEL_STRICT_NAMES) || defined(__ASSEMBLY__))
/* GPIO registers */
struct gpio_regs {
	u32 gpio_dr;	/* data */
	u32 gpio_dir;	/* direction */
	u32 gpio_psr;	/* pad satus */
};
#endif

#define IMX_GPIO_NR(port, index)		((((port)-1)*32)+((index)&31))

/* gpio and gpio based interrupt handling */
#define GPIO_DR                 0x00
#define GPIO_GDIR               0x04
#define GPIO_PSR                0x08
#define GPIO_ICR1               0x0C
#define GPIO_ICR2               0x10
#define GPIO_IMR                0x14
#define GPIO_ISR                0x18
#define GPIO_INT_LOW_LEV        0x0
#define GPIO_INT_HIGH_LEV       0x1
#define GPIO_INT_RISE_EDGE      0x2
#define GPIO_INT_FALL_EDGE      0x3
#define GPIO_INT_NONE           0x4

#endif
