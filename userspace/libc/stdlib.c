#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEAP_CAPACITY (64 * 1024)
#define HEAP_ALIGNMENT 16

typedef union {
    size_t size;
    long double floating;
    void *pointer;
} AllocationHeader;

static unsigned char heap[HEAP_CAPACITY] __attribute__((aligned(HEAP_ALIGNMENT)));
static size_t heap_used;

void *malloc(size_t size)
{
    if (size == 0) size = 1;
    if (size > (size_t)-1 - sizeof(AllocationHeader) - HEAP_ALIGNMENT) return 0;
    size_t start = (heap_used + HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1);
    size_t total = sizeof(AllocationHeader) + size;
    if (start > HEAP_CAPACITY || total > HEAP_CAPACITY - start) return 0;
    AllocationHeader *header = (AllocationHeader *)(void *)(heap + start);
    header->size = size;
    heap_used = start + total;
    return header + 1;
}

void *calloc(size_t count, size_t size)
{
    if (size && count > (size_t)-1 / size) return 0;
    size_t total = count * size;
    void *allocation = malloc(total);
    if (allocation) memset(allocation, 0, total);
    return allocation;
}

void *realloc(void *pointer, size_t size)
{
    if (!pointer) return malloc(size);
    if (size == 0) return 0;
    AllocationHeader *header = (AllocationHeader *)pointer - 1;
    void *replacement = malloc(size);
    if (!replacement) return 0;
    size_t copied = header->size < size ? header->size : size;
    memcpy(replacement, pointer, copied);
    return replacement;
}

void free(void *pointer)
{
    (void)pointer;
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

_Noreturn void abort(void)
{
    _exit(134);
}

_Noreturn void exit(int status)
{
    _exit(status);
}