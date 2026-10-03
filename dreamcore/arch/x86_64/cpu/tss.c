#include <stdint.h>
#include "tss.h"

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} Tss;

extern uint64_t gdt64[];
extern unsigned char user_kernel_stack[];

static Tss tss __attribute__((aligned(16)));

void tss_init(void) {
    uint64_t base = (uint64_t)&tss;
    uint64_t descriptor = 0;
    tss.rsp0 = (uint64_t)user_kernel_stack + 4096;
    tss.iomap_base = sizeof(Tss);

    descriptor |= sizeof(Tss) - 1;
    descriptor |= (base & 0xFFFFFF) << 16;
    descriptor |= 0x89ULL << 40;
    descriptor |= ((base >> 24) & 0xFF) << 56;
    gdt64[5] = descriptor;
    gdt64[6] = base >> 32;
    __asm__ volatile ("ltr %0" : : "r"((uint16_t)0x28));
}