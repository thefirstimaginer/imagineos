#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    int status;
    long login_pid;

    for (;;) {
        write(STDOUT_FILENO, "ImagineOS console getty\n", 24);
        write(STDOUT_FILENO, "Starting login service...\n", 26);
        login_pid = exec_service("login.service");
        if (login_pid < 0) {
            puts("[FAIL] login service could not be started.");
            exit(1);
        }
        status = 0;
        if (waitpid(login_pid, &status, 0) < 0) {
            puts("[FAIL] login service could not be reaped.");
            exit(1);
        }
    }
}
