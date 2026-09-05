#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    puts("ImagineOS userspace init started.");
    puts("Running outside the kernel through libc syscalls.");
    exit(0);
}