#include <stdint.h>
#include "syscall.h"

#define IA32_EFER 0xC0000080u
#define IA32_STAR 0xC0000081u
#define IA32_LSTAR 0xC0000082u
#define IA32_FMASK 0xC0000084u
#define EFER_SCE (1u << 0)
#define RFLAGS_INTERRUPT (1u << 9)

extern void syscall_entry(void);

static void write_msr(uint32_t number, uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ volatile ("wrmsr" : : "c"(number), "a"(low), "d"(high));
}

static uint64_t read_msr(uint32_t number) {
    uint32_t low;
    uint32_t high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(number));
    return ((uint64_t)high << 32) | low;
}

void syscall_init(void) {
    write_msr(IA32_EFER, read_msr(IA32_EFER) | EFER_SCE);
    write_msr(IA32_STAR, ((uint64_t)0x10 << 48) | ((uint64_t)0x08 << 32));
    write_msr(IA32_LSTAR, (uint64_t)syscall_entry);
    write_msr(IA32_FMASK, RFLAGS_INTERRUPT);
}