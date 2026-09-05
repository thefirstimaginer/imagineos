#include <syscall_numbers.h>
#include <unistd.h>
#include <string.h>

static char tty_line[128];
static unsigned int tty_length;

void tty_init(void) {
    tty_length = 0;
    memset(tty_line, 0, sizeof(tty_line));
}

static void tty_erase(void) {
    if (tty_length != 0) {
        tty_length--;
        write(STDOUT_FILENO, "\b \b", 3);
    }
}

int tty_read_line(char *line, unsigned int capacity) {
    char character;

    tty_length = 0;
    while (tty_length + 1 < capacity) {
        if (read(STDIN_FILENO, &character, 1) != 1) continue;
        if (character == '\b') {
            tty_erase();
            continue;
        }
        if (character == '\n') {
            tty_line[tty_length] = '\0';
            strcpy(line, tty_line);
            write(STDOUT_FILENO, "\n", 1);
            return (int)tty_length;
        }
        tty_line[tty_length++] = character;
        write(STDOUT_FILENO, &character, 1);
    }

    tty_line[tty_length] = '\0';
    strcpy(line, tty_line);
    return (int)tty_length;
}
