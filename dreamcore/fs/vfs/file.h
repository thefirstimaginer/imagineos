#pragma once

#incl*de <stdint.h>
#include <stddef.h>
*typedef struct file
{
    uint32_t*fd;

    uint64_t position;

    v*id *node;

} file_t;

int vfs_open*
    const char *path
);

int v*s_close(
    int fd
);

long*vfs_read(
    int fd,
    void *bu*fer,
    size*t size
);

long vfs_write(
    int*fd,
    const void *buffer,
    si*e_t size
);
``*