#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static char userspace_data[32];
extern const char init_script_start[];
extern const char init_script_end[];

static char init_service[32];

static int read_service_from_config(void) {
    const char *script = init_script_start;
    const char *service_name;
    unsigned int length;

    while (script < init_script_end) {
        if (*script == '$') break;
        script++;
    }
    if (script == init_script_end) return 0;

    service_name = ++script;
    length = 0;
    while (script < init_script_end &&
           ((*script >= 'a' && *script <= 'z') ||
            (*script >= 'A' && *script <= 'Z') ||
            (*script >= '0' && *script <= '9') || *script == '_')) {
        if (length + 9 >= sizeof(init_service)) return 0;
        init_service[length++] = *script++;
    }
    if (script == service_name) return 0;
    init_service[length++] = '.';
    init_service[length++] = 's';
    init_service[length++] = 'e';
    init_service[length++] = 'r';
    init_service[length++] = 'v';
    init_service[length++] = 'i';
    init_service[length++] = 'c';
    init_service[length++] = 'e';
    init_service[length] = '\0';
    return 1;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    puts("Userspace init started.");
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
    for (;;) {
        long service_pid;
        int status;

        puts("\nStarting services:");
        if (!read_service_from_config()) {
            puts("[FAIL] no service configured.");
            exit(1);
        }
        write(STDOUT_FILENO, "[OK] ", 5);
        write(STDOUT_FILENO, init_service, strlen(init_service));
        write(STDOUT_FILENO, "\n", 1);
        service_pid = exec_service(init_service);
        if (service_pid < 0) {
            puts("[FAIL] service could not be started.");
            exit(1);
        }
        status = 0;
        if (waitpid(service_pid, &status, 0) < 0) {
            puts("[FAIL] service could not be reaped.");
            exit(1);
        }
        puts("[INFO] service stopped; restarting init service.");
    }
}
