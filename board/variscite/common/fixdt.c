// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 Variscite Ltd.
 */

#include <fdtdec.h>

#include "fixdt.h"

int var_delete_dt_node(const char *node_name, const char *parent_node_path, void *fdt_blob)
{
	int node_offset, parent_node_offset;

	parent_node_offset = fdt_path_offset(fdt_blob, parent_node_path);
	if (parent_node_offset < 0)
		return parent_node_offset;

	node_offset = fdt_subnode_offset(fdt_blob, parent_node_offset, node_name);
	if (node_offset < 0)
		return node_offset;

	return fdt_del_node(fdt_blob, node_offset);
}
