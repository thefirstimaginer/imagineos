#include "print.h"
#include "shell.h"
#include "modules.h"
#include "libraries/string.h"

#define SHELL_HISTORY_SIZE 24
#define SHELL_HISTORY_LENGTH 128

static char command_history[SHELL_HISTORY_SIZE][SHELL_HISTORY_LENGTH];
static int history_count = 0;
static int history_write = 0;
static int history_position = -1;

static void history_add(const char* line) {
    strncpy(command_history[history_write], line, SHELL_HISTORY_LENGTH - 1);
    command_history[history_write][SHELL_HISTORY_LENGTH - 1] = '\0';
    history_write = (history_write + 1) % SHELL_HISTORY_SIZE;
    if (history_count < SHELL_HISTORY_SIZE) history_count++;
    history_position = -1;
}

const char* shell_history_up(void) {
    if (history_count == 0) return "";
    if (history_position < history_count - 1) history_position++;
    return command_history[(history_write + SHELL_HISTORY_SIZE - 1 - history_position) % SHELL_HISTORY_SIZE];
}

const char* shell_history_down(void) {
    if (history_position < 0) return "";
    history_position--;
    if (history_position < 0) return "";
    return command_history[(history_write + SHELL_HISTORY_SIZE - 1 - history_position) % SHELL_HISTORY_SIZE];
}

static void skip_command_spaces(char** text) {
    while (**text == ' ') (*text)++;
}

void shell_execute_line(const char* line) {
    char command[32] = {0};
    char arguments[224] = {0};
    char* cursor = (char*)line;
    int command_length = 0;

    skip_command_spaces(&cursor);
    while (cursor[command_length] != '\0' && cursor[command_length] != ' ' && command_length < 31) {
        command[command_length] = cursor[command_length];
        command_length++;
    }
    command[command_length] = '\0';
    cursor += command_length;
    skip_command_spaces(&cursor);
    strncpy(arguments, cursor, sizeof(arguments) - 1);

    if (command[0] == '\0') return;

    history_add(line);

    if (strcmp(command, "history") == 0) {
        int first = history_count == SHELL_HISTORY_SIZE ? history_write : 0;
        for (int offset = 0; offset < history_count; offset++) {
            int entry = (first + offset) % SHELL_HISTORY_SIZE;
            print_uint64_dec((uint64_t)(offset + 1));
            print_str("  ");
            print_str(command_history[entry]);
            print_str("\n");
        }
        return;
    }

    for (int index = 0; index < modules_count; index++) {
        if (strcmp(modules[index].name, command) == 0) {
            if (modules[index].run) modules[index].run(arguments);
            if (col != 0) print_str("\n");
            return;
        }
    }

    print_str("Comando nao encontrado: ");
    print_str(command);
    print_str("\n");
}

void shell_init(void) {
}

void shell_run(char* args) {
    (void)args;
}
