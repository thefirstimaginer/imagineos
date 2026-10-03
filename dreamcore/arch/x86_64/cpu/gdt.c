#include <stdint.h>

struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdtr;

uint64_t gdt64[7] = {
    0x0000000000000000ULL,
    0x00AF9A000000FFFFULL,
    0x00CF92000000FFFFULL,
    0x00CFF2000000FFFFULL,
    0x00AFFA000000FFFFULL,
    0,
    0
};

void gdt_load(void)
{
    gdtr.limit = sizeof(gdt64) - 1;
    gdtr.base  = (uint64_t)gdt64;

    asm volatile ("lgdt %0" : : "m"(gdtr));
}