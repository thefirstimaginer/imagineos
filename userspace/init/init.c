#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static char userspace_data[32];
extern const char init_script_start[];
extern const char init_script_end[];

static char init_service[32];

static int read_service_from_script(void) {
    const char *script = init_script_start;
    unsigned int length = 0;

    while (script < init_script_end && (*script == ' ' || *script == '\n')) script++;
    while (script < init_script_end && *script != ' ' && *script != '\n' &&
           length + 1 < sizeof(init_service)) {
        init_service[length++] = *script++;
    }
    init_service[length] = '\0';
    return length != 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    puts("userspace init started.");
    puts("Running outside the kernel through libc syscalls.");

    userspace_data[0] = 'U';
    userspace_data[1] = 'S';
    if (userspace_data[0] == 'U' && userspace_data[1] == 'S') {
        puts("ring 3 user memory is writable.");
    } else {
        puts("[FAIL] ring 3 user memory test failed.");
        exit(1);
    }

    if (write(99, "x", 1) == -1 && errno == EBADF) {
        puts("syscall error returned through libc errno.");
    } else {
        puts("[FAIL] syscall error path failed.");
        exit(1);
    }

    puts("ring 3 smoke test complete.");
    puts("\nStarting services:");
    if (read_service_from_script()) {
        write(STDOUT_FILENO, "[OK] ", 5);
        write(STDOUT_FILENO, init_service, strlen(init_service));
        write(STDOUT_FILENO, "\n", 1);
        if (exec_service(init_service) < 0) {
            puts("[FAIL] service could not be started.");
            exit(1);
        }
    }
    return 0;
}
