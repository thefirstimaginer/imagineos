#ifndef IMAGINEOS_KERNEL_PAGING_H
#define IMAGINEOS_KERNEL_PAGING_H

#include <stdint.h>

uint64_t paging_create_user_space(void);
void paging_destroy_user_space(uint64_t cr3);
void paging_copy_user_image(uint64_t cr3);
uint64_t paging_user_physical(uint64_t cr3);
void paging_activate(uint64_t cr3);

#endif
