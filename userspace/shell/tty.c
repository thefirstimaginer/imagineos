#include <syscall_numbers.h>
#include <unistd.h>
#include <string.h>

static char tty_line[128];
static unsigned int tty_length;
static int tty_cursor_visible;

void tty_init(void) {
    tty_length = 0;
    tty_cursor_visible = 0;
    memset(tty_line, 0, sizeof(tty_line));
}

static void tty_cursor_hide(void) {
    if (tty_cursor_visible) {
        write(STDOUT_FILENO, "\b", 1);
        tty_cursor_visible = 0;
    }
}

static void tty_cursor_show(void) {
    if (!tty_cursor_visible) {
        write(STDOUT_FILENO, "_", 1);
        tty_cursor_visible = 1;
    }
}

static void tty_cursor_blink(void) {
    if (tty_cursor_visible) tty_cursor_hide();
    else tty_cursor_show();
}

static void tty_erase(void) {
    if (tty_length != 0) {
        tty_cursor_hide();
        tty_length--;
        write(STDOUT_FILENO, "\b", 1);
        tty_cursor_show();
    }
}

int tty_read_line(char *line, unsigned int capacity) {
    char character;
    unsigned long next_blink = get_ticks() + 150;

    if (capacity == 0) return 0;
    if (capacity > sizeof(tty_line)) capacity = sizeof(tty_line);
    tty_length = 0;
    tty_cursor_show();
    for (;;) {
        if (read_nonblock(STDIN_FILENO, &character, 1) != 1) {
            if (get_ticks() >= next_blink) {
                tty_cursor_blink();
                next_blink = get_ticks() + 150;
            }
            continue;
        }
        tty_cursor_hide();
        if (character == '\b') {
            tty_erase();
            continue;
        }
        if (character == '\n') {
            tty_line[tty_length] = '\0';
            strcpy(line, tty_line);
            write(STDOUT_FILENO, "\n", 1);
            tty_cursor_visible = 0;
            return (int)tty_length;
        }
        if (tty_length + 1 < capacity) {
            tty_line[tty_length++] = character;
        }
        write(STDOUT_FILENO, &character, 1);
        tty_cursor_show();
        next_blink = get_ticks() + 150;
    }
}
