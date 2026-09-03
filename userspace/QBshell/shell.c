#include "print.h"
#include "string.h"
#include "modules.h"

void qbs_main() {

}

void qbs_commands() {

	if (strcmp(cmd_name, "help") == 0) {
		print_str("QBSHELL - vR1 - Aug 2026 Release\n");
        print_str("Copyright (c) 2026, TeamImagine\n\n");

        print_str("Available Commands:\n");
        print_str("  pwd  - Show Current Dir\n");
        print_str("  list - List directory content\n");
        print_str("  cd   - Change Dir\n");
        print_str("  exec - Change actual shell proccess by another program.\n");
        print_str("  exit - Exit the shell\n");
        print_str("  kill - Force stop of a current task\n");
        print_str("  echo - Print message on the screen\n");
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
    else {
        print_str("Unknown command: ");
        print_str(cmd_name);
        print_str("\n");
    }

}

void qbs_echo_prompt() {
    print_str("root@tty:$ ");
    //print_str("\u@\h:\w\$ ");
}