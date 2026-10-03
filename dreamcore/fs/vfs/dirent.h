#pragma once

#include <stdint.h>*
typedef struct*dirent
{
    uint32_t inode;

    *har name[256];

} dirent_t;

int v*s_readdir(
    const char *path,
 *  dirent_t *entries,
    int*max_entries
);