/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2020 Philippe Reynes <philippe.reynes@softathome.com>
 */

#ifndef __UDP
#define __UDP

#include <linux/types.h>

/* udp_ops flags */
#define UDP_OPS_NO_IPADDR	(1U << 0)  /* skip 'ipaddr not set' prereq */

/**
 * struct udp_ops - function to handle udp packet
 *
 * This structure provides the function to handle udp packet in
 * the network loop.
 *
 * @prereq: callback called to check the requirement
 * @start: callback called to start the protocol/feature
 * @data: pointer to store private data (used by prereq and start)
 * @flags: capability flags (UDP_OPS_NO_IPADDR, ...)
 */
struct udp_ops {
	int (*prereq)(void *data);
	int (*start)(void *data);
	void *data;
	u32 flags;
};

int udp_prereq(void);

/**
 * udp_needs_ipaddr() - Whether the registered udp_ops needs a local ipaddr.
 *
 * Used by net/net.c so that protocols which can operate without a local IP
 * (e.g. udp_wait, which only ever receives) do not trip the
 * "*** ERROR: `ipaddr' not set" prereq check.
 *
 * Return: true if ipaddr is required, false otherwise.
 */
bool udp_needs_ipaddr(void);

int udp_start(void);

/**
 * udp_loop() - network loop for udp protocol
 *
 * Launch a network loop for udp protocol and use callbacks
 * provided in parameter @ops to initialize the loop, and then
 * to handle udp packet.
 *
 * @ops: udp callback
 * @return: 0 if success, otherwise < 0 on error
 */
int udp_loop(struct udp_ops *ops);

#endif
