#include "kmalloc.h"
#include <stdint.h>

#define KERNEL_HEAP_SIZE (64 * 1024)

static unsigned char kernel_heap[KERNEL_HEAP_SIZE];
static size_t kernel_heap_offset;

void *kmalloc(size_t size) {
    size_t aligned_size = (size + 7) & ~((size_t)7);
    if (aligned_size > KERNEL_HEAP_SIZE - kernel_heap_offset) return NULL;
    void *result = &kernel_heap[kernel_heap_offset];
    kernel_heap_offset += aligned_size;
    return result;
}

void kfree(void *pointer) {
    (void)pointer;
}