#include <stdio.h>
#include <syscall_numbers.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void tty_init(void);
int tty_read_line(char *line, unsigned int capacity);

static void shell_write(const char *text) {
    write(STDOUT_FILENO, text, strlen(text));
}

static int starts_with(const char *text, const char *prefix) {
    while (*prefix != '\0') {
        if (*text++ != *prefix++) return 0;
    }
    return 1;
}

static void shell_help(void) {
    shell_write("help       show commands\n");
    shell_write("echo TEXT  print text\n");
    shell_write("test       test userspace syscalls\n");
    shell_write("clear      clear the terminal\n");
    shell_write("exit       stop the shell\n");
}

int main(void) {
    char line[128];

    tty_init();
    shell_write("\nWelcome to Imagine System R1!\n\n");

    for (;;) {
        shell_write("$ ");
        tty_read_line(line, sizeof(line));

        if (strcmp(line, "help") == 0) {
            shell_help();
        } else if (strcmp(line, "test") == 0) {
            shell_write("[OK] shell is running in userspace\n");
        } else if (strcmp(line, "clear") == 0) {
            syscall6(SYS_CLEAR_TERMINAL, 0, 0, 0, 0, 0, 0);
        } else if (strcmp(line, "exit") == 0) {
            shell_write("shell stopped\n");
            exit(0);
        } else if (strcmp(line, "echo") == 0) {
            shell_write("usage: echo TEXT\n");
        } else if (starts_with(line, "echo ")) {
            shell_write(line + 5);
            shell_write("\n");
        } else if (line[0] != '\0') {
            shell_write("command not found: ");
            shell_write(line);
            shell_write("\n");
        }
    }
}
