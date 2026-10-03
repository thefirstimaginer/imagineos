#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum
{
    VFS_NODE_FILE,
    VFS_NODE_DIR,
    VFS_NODE_DEVICE
} vfs_node_type_t;

typedef struct vfs_node
{
    char name[256];
    vfs_node_type_t type;
    uint64_t size;
    void *data;

    struct vfs_node *parent;
    struct vfs_node *children;
    struct vfs_node *next;
} vfs_node_t;

int vfs_init(void);
vfs_node_t *vfs_root(void);
vfs_node_t *vfs_lookup(const char *path);
int vfs_create(const char *path, vfs_node_type_t type);
int vfs_remove(const char *path);
int vfs_mkdir(const char *path);