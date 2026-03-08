#include "print.h"
#include "shellMain.h"
#include "x86_64/rtc.h"
#include "graphics.h"

/* precisamos de memset, que está declarado em libimagine.h */
#include "libraries/libimagine.h"   // inclui string/math e protótipos de memória

#include "modules.h"

char last_command[128] = {0};
static char input_buffer[256] = {0};
static int input_index = 0;

static int shell_x = 10;
static int shell_y = 170;

void shell_init() {
    print_clear();
    
    print_str("  __________________________________________________________  \n");
    print_str("   ___                       _             ___  ____  \n");
    print_str("  |_ _|_ __ ___   __ _  __ _(_)_ __   ___ / _ \\/ ___| \n");
    print_str("   | || '_ ` _ \\ / _` |/ _` | | '_ \\ / _ \\ | | \\___ \\ \n");
    print_str("   | || | | | | | (_| | (_| | | | | |  __/ |_| |___) |\n");
    print_str("  |___|_| |_| |_|\\__,_|\\__, |_|_| |_|\\___|\\___/|____/ \n");
    print_str("                       |___/                   \n");
    print_str("  __________________________________________________________  \n");
    
    print_str(" ImagineOS R1 | Kernel: x86_64 | Text Mode (Graphics disabled)\n");
    print_str(" Type 'help' to see available commands.\n");
    
    shell_print_prompt();
    input_index = 0;
}

void shell_print_prompt() {
    print_str("main@os > ");
}

void shell_handle_enter(void) {
    char* cmd = input_buffer;
    
    char cmd_name[32] = {0};
    char* args = "";
    
    char* space = strchr(cmd, ' ');
    if (space) {
        strncpy(cmd_name, cmd, space - cmd);
        args = space + 1;
    } else {
        strncpy(cmd_name, cmd, 31);
    }
    
    if (cmd_name[0] == '\0') {
        print_str("\n");
        shell_print_prompt();
        input_index = 0;
        return;
    }

    strncpy(last_command, cmd, 127);
    modules_load();

    if (strcmp(cmd_name, "help") == 0) {
        print_str("Available Commands:\n");
        print_str("  clear      - Clear the screen\n");
        print_str("  date       - Show current date and time\n");
        print_str("  history    - Show last command\n");
        print_str("  ver        - Show OS version\n");
    }
    else if (strcmp(cmd_name, "clear") == 0) {
        print_clear();
        shell_print_prompt();
        input_index = 0;
        return;
    }
    else if (strcmp(cmd_name, "date") == 0) {
        uint8_t day = rtc_get_value(RTC_REGISTER_DAY);
        uint8_t month = rtc_get_value(RTC_REGISTER_MONTH);
        uint8_t year = rtc_get_value(RTC_REGISTER_YEAR);
        uint8_t hour = rtc_get_value(RTC_REGISTER_HOURS);
        uint8_t minute = rtc_get_value(RTC_REGISTER_MINUTES);
        
        print_str("Date: 20");
        print_uint64_hex(year);
        print_str("/");
        print_uint64_hex(month);
        print_str("/");
        print_uint64_hex(day);
        print_str(" ");
        print_uint64_hex(hour);
        print_str(":");
        print_uint64_hex(minute);
        print_str("\n");
    }
    else if (strcmp(cmd_name, "history") == 0) {
        print_str("Last command: ");
        if (last_command[0] != '\0') {
            print_str(last_command);
        }
        print_str("\n");
    }
    else if (strcmp(cmd_name, "ver") == 0) {
        print_str("ImagineOS R1 (Text Mode)\n");
    }
    else {
        print_str("Unknown command: ");
        print_str(cmd_name);
        print_str("\n");
    }

    print_str("\n");
    shell_print_prompt();
    input_index = 0;
    memset(input_buffer, 0, 256);
}

void shell_add_char(char c) {
    if (c == '\b') {
        if (input_index > 0) {
            input_index--;
            input_buffer[input_index] = '\0';
            print_str("\b \b");  // backspace
        }
    } else if (c == '\n') {
        input_buffer[input_index] = '\0';
        shell_handle_enter();
    } else if (input_index < 255) {
        input_buffer[input_index] = c;
        input_index++;
        input_buffer[input_index] = '\0';
        print_char(c);
    }
}