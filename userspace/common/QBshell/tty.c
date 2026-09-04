#include "print.h"
#include "tty.h"
#include "shell.h"
#include "userspace/login.h"
#include "libraries/libimagine.h"

static char input_buffer[256] = {0};
static int input_index = 0;

void shell_print_prompt(void) {
    print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
    print_str((char*)login_get_username());
    print_str("@");
    print_str((char*)login_get_hostname());
    print_str(":~$ ");
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    shell_prompt_row = row;
    shell_prompt_col = col;
    enable_cursor(0, 15);
    set_cursor(col, row);
}

void terminal_init(void) {
    input_index = 0;
    memset(input_buffer, 0, sizeof(input_buffer));
}

void terminal_start(void) {
    terminal_init();
    shell_print_prompt();
}

static void terminal_replace_input(const char* text) {
    while (input_index > 0) {
        input_index--;
        input_buffer[input_index] = '\0';
        backspace();
    }

    strncpy(input_buffer, text, sizeof(input_buffer) - 1);
    input_buffer[sizeof(input_buffer) - 1] = '\0';
    input_index = (int)strlen(input_buffer);
    print_str(input_buffer);
}

void terminal_history_up(void) {
    terminal_replace_input(shell_history_up());
}

void terminal_history_down(void) {
    terminal_replace_input(shell_history_down());
}

void terminal_input(char c) {
    if (c == '\b') {
        if (input_index > 0) {
            input_index--;
            input_buffer[input_index] = '\0';
            backspace();
        }
        return;
    }

    if (c == '\r') return;

    if (c == '\n') {
        input_buffer[input_index] = '\0';
        print_str("\n");
        shell_execute_line(input_buffer);
        input_index = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        shell_print_prompt();
        return;
    }

    if (c >= 32 && c < 127 && input_index < (int)sizeof(input_buffer) - 1) {
        input_buffer[input_index++] = c;
        print_char(c);
    }
}

void shell_add_char(char c) {
    terminal_input(c);
}
