#include "idt.h"
#include "scheduler.h"
#include "syscall.h"
#include "tss.h"
#include "print.h"
#include "input.h"

#include <stdbool.h>
#include <stdint.h>
#include <limine.h>


__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start[] = {
    LIMINE_REQUESTS_START_MARKER
};

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = {
    LIMINE_BASE_REVISION(3)
};

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};


__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end[] = {
    LIMINE_REQUESTS_END_MARKER
};

void kernel_main(void) {
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision))
        hcf();

    if (framebuffer_request.response &&
    framebuffer_request.response->framebuffer_count)
    {
        video_init(
            framebuffer_request.response->framebuffers[0]
        );

        enable_graphics_print();
    }
    gdt_load();
    tss_init();
    idt_init();

    syscall_init();

    process_init();
    scheduler_init();

    input_init();

    print_str("Dreamcore Kernel Release 1 - Alpha 1\n");

    if (condition)
    {
        /* code */
    }
    

    for (;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
