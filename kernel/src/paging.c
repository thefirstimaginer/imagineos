#include "paging.h"
#include <stdint.h>

#define PAGE_PRESENT 0x001ULL
#define PAGE_WRITABLE 0x002ULL
#define PAGE_USER 0x004ULL
#define PAGE_LARGE 0x080ULL
#define USER_IMAGE_BASE 0x400000ULL
#define USER_IMAGE_SIZE 0x200000ULL
#define USER_PHYSICAL_BASE 0x02000000ULL
#define MAX_USER_SPACES 16

static uint64_t page_l4[MAX_USER_SPACES][512] __attribute__((aligned(4096)));
static uint64_t page_l3[MAX_USER_SPACES][512] __attribute__((aligned(4096)));
static uint64_t page_l2[MAX_USER_SPACES][4][512] __attribute__((aligned(4096)));
static uint32_t space_count;

static void clear_page(void *page) {
    uint64_t *entries = page;
    unsigned int index;
    for (index = 0; index < 512; index++) entries[index] = 0;
}

uint64_t paging_create_user_space(void) {
    uint32_t slot = space_count++;
    unsigned int directory;
    unsigned int entry;
    uint64_t user_physical;

    if (slot >= MAX_USER_SPACES) return 0;
    clear_page(page_l4[slot]);
    clear_page(page_l3[slot]);
    for (directory = 0; directory < 4; directory++) {
        clear_page(page_l2[slot][directory]);
    }

    page_l4[slot][0] = (uint64_t)(uintptr_t)page_l3[slot] | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    for (directory = 0; directory < 4; directory++) {
        page_l3[slot][directory] = (uint64_t)(uintptr_t)page_l2[slot][directory] |
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        for (entry = 0; entry < 512; entry++) {
            uint64_t physical = ((uint64_t)directory * 512 + entry) * 0x200000ULL;
            uint64_t flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_LARGE;
            if (directory == 0 && (entry == 2 || entry == 1)) {
                user_physical = USER_PHYSICAL_BASE + (uint64_t)slot * USER_IMAGE_SIZE;
                if (entry == 2) physical = user_physical;
                flags |= PAGE_USER;
            }
            page_l2[slot][directory][entry] = physical | flags;
        }
    }
    return (uint64_t)(uintptr_t)page_l4[slot];
}

uint64_t paging_user_physical(uint64_t cr3) {
    unsigned int slot;
    for (slot = 0; slot < space_count; slot++) {
        if ((uint64_t)(uintptr_t)page_l4[slot] == cr3) {
            return USER_PHYSICAL_BASE + (uint64_t)slot * USER_IMAGE_SIZE;
        }
    }
    return 0;
}

void paging_copy_user_image(uint64_t cr3) {
    uint64_t physical = paging_user_physical(cr3);
    uint8_t *source = (uint8_t *)(uintptr_t)USER_IMAGE_BASE;
    uint8_t *destination = (uint8_t *)(uintptr_t)physical;
    uint64_t index;

    if (physical == 0) return;
    for (index = 0; index < USER_IMAGE_SIZE; index++) destination[index] = source[index];
}

void paging_activate(uint64_t cr3) {
    if (cr3 != 0) __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}
