// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 Variscite Ltd.
 */

#ifndef __VAR_FIXDT_H
#define __VAR_FIXDT_H

int var_delete_dt_node(const char *node_name,
		       const char *parent_node_path,
		       void *fdt_blob);

#endif /* __VAR_FIXDT_H */
