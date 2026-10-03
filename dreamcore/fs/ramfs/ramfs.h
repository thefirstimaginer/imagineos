#pragma once

#include "../vfs/vfs.h"

int ramfs_init(void);

vfs_node_t *ramfs_create_node(
    const char *name,
    vfs_node_type_t type
);