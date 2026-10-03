#ifndef IMAGINEOS_KMALLOC_H
#define IMAGINEOS_KMALLOC_H

#include <stddef.h>

void *kmalloc(size_t size);
void kfree(void *pointer);

#endif