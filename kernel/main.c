#include "video.h"
#include "idt.h"
#include "scheduler.h"
#include "syscall.h"
#include "user.h"
#include "tss.h"
#include "print.h"
#include "input.h"

void kernel_main(uint64_t multiboot_info) {
    print_clear();
    print_str("Dreamcore Kernel Release 1 - Alpha 1\n");
    /* Configure privileged services before enabling hardware interrupts. */
    syscall_init();
    print_str("Syscall MSRs Configured!\n");
    /* Configure the TSS before enabling interrupts. */
    tss_init();
    print_str("TSS Configured!\n");
    video_init();
    scheduler_init();
    process_init();

    input_init();
    idt_init();
    print_str("IDT and scheduler configured!\n");

     /* Do not let the timer interrupt the kernel while the first address space
         and its process frame are being prepared. user_enter enables interrupts
         as part of the ring-3 iret frame. */
     __asm__ volatile ("cli" : : : "memory");
    if (user_init_from_multiboot(multiboot_info) != 0) {
        print_str("[FAIL] init.elf not found or invalid\n");
        while (1) asm volatile("hlt");
    }

    while (1) {
        asm volatile("hlt");
    }
}
