#include <stdio.h>
#include <unistd.h>
#include <string.h>

static int login_cursor_visible;

static void login_cursor_hide(void) {
    if (login_cursor_visible) {
        write(STDOUT_FILENO, "\b", 1);
        login_cursor_visible = 0;
    }
}

static void login_cursor_show(void) {
    if (!login_cursor_visible) {
        write(STDOUT_FILENO, "_", 1);
        login_cursor_visible = 1;
    }
}

static void login_cursor_blink(void) {
    if (login_cursor_visible) login_cursor_hide();
    else login_cursor_show();
}

static int read_username(char *username, unsigned int capacity) {
    unsigned int length = 0;
    char character;
    unsigned long next_blink = get_ticks() + 150;

    write(STDOUT_FILENO, "login: ", 7);
    login_cursor_show();
    if (capacity == 0) return 0;
    for (;;) {
        if (read_nonblock(STDIN_FILENO, &character, 1) != 1) {
            if (get_ticks() >= next_blink) {
                login_cursor_blink();
                next_blink = get_ticks() + 150;
            }
            continue;
        }
        login_cursor_hide();
        if (character == '\n') break;
        if (character == '\b') {
            if (length != 0) {
                length--;
                write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }
        if (length + 1 < capacity) username[length++] = character;
        write(STDOUT_FILENO, &character, 1);
        login_cursor_show();
        next_blink = get_ticks() + 150;
    }
    login_cursor_hide();
    username[length] = '\0';
    write(STDOUT_FILENO, "\n", 1);
    return (int)length;
}

int main(void) {
    char username[32];
    int status;
    long shell_pid;

    puts("Login service started.");
    puts("\nDevelopment mode: no password database is configured.");
    for (;;) {
        if (read_username(username, sizeof(username)) < 0) {
            puts("[FAIL] console read failed.");
            exit(1);
        }
        if (username[0] == '\0') {
            puts("Please enter a username.");
            continue;
        }
        write(STDOUT_FILENO, "Hello, ", 7);
        write(STDOUT_FILENO, username, strlen(username));
        write(STDOUT_FILENO, "!\n", 2);
        shell_pid = exec_service("shell.service");
        if (shell_pid < 0) {
            puts("[FAIL] shell service could not be started.");
            exit(1);
        }
        status = 0;
        if (waitpid(shell_pid, &status, 0) < 0) {
            puts("[FAIL] shell service could not be reaped.");
            exit(1);
        }
    }
}
