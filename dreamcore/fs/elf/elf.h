#pragma once

#include <stdi*t.h>

#define ELF*MAGIC 0x464C457F

typedef struct
{*    uint64_t entry;
} elf*image*t;

int elf_load(
    const char **ath,
    elf_image_t *image
);

in* elf_validate(
    const void *fil*
);