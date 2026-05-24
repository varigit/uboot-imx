/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * UDP wait trigger header
 *
 * Ported from real-time-edge-uboot feature/axisb/udp_tftp_boot.
 */

#ifndef __UDP_WAIT_H__
#define __UDP_WAIT_H__

#include <linux/types.h>

/* Configuration for udp_wait (set by the udp_wait command before udp_loop) */
extern int udp_wait_port;
extern unsigned long udp_wait_timeout;

/**
 * udp_wait_prereq() - Check prerequisites for udp_wait
 * @data: Private data (unused)
 * Return: 0 if prerequisites met, 1 otherwise
 */
int udp_wait_prereq(void *data);

/**
 * udp_wait_start() - Start UDP wait listener
 * @data: Private data (unused)
 * Return: 0 on success
 */
int udp_wait_start(void *data);

#endif /* __UDP_WAIT_H__ */
