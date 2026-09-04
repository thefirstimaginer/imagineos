#include "init.h"
#include "process.h"
#include "modules.h"
#include "shell.h"
#include "print.h"
#include "rtc.h"

extern void login_prompt(void);
extern void calc_run(char* args);
extern void liteinterp(char* args);
extern void halt_run(char* args);
extern void ver_run(char* args);
extern void help_run(char* args);
extern void clear_run(char* args);
extern void video_run(void);
extern void ps_run(char* args);

static void print_two_digits(uint8_t value) {
    if (value < 10) print_char('0');
    print_uint64_dec(value);
}

static void init_wait_seconds(uint8_t seconds) {
    uint8_t start = rtc_seconds();
    while ((uint8_t)(rtc_seconds() - start) < seconds) {
    }
}

static void init_print_datetime(void) {
    print_clear();
    print_str("INIT v0.1 (");
    print_two_digits(rtc_get_value(RTC_REGISTER_DAY));
    print_str("/");
    print_two_digits(rtc_get_value(RTC_REGISTER_MONTH));
    print_str("/20");
    print_two_digits(rtc_get_value(RTC_REGISTER_YEAR));
    print_str(") (");
    print_two_digits(rtc_get_value(RTC_REGISTER_HOURS));
    print_str(":");
    print_two_digits(rtc_get_value(RTC_REGISTER_MINUTES));
    print_str(":");
    print_two_digits(rtc_seconds());
    print_str(")\n\n");
}

static void init_print_modules(void) {
    print_str("STARTING MODULES:\n");
    for (int index = 0; index < modules_count; index++) {
        print_str("  [OK] ");
        print_str((char*)modules[index].name);
        print_str("\n");
    }
    print_str("\n");
}

void init_system(void) {
    process_init();
    enable_cursor(0, 15);
    process_create_named("shell", (void (*)(void))shell_run);
    process_create_named("calc", (void (*)(void))calc_run);
    process_create_named("liteinterp", (void (*)(void))liteinterp);
    process_create_named("halt", (void (*)(void))halt_run);
    process_create_named("ver", (void (*)(void))ver_run);
    process_create_named("help", (void (*)(void))help_run);
    process_create_named("clear", (void (*)(void))clear_run);
    process_create_named("video", (void (*)(void))video_run);
    process_create_named("ps", (void (*)(void))ps_run);

    init_print_datetime();
    init_print_modules();
    modules_load();
    print_str("Welcome to Imagine OS R1 JESSICA!\n");
    init_wait_seconds(1);
    print_clear();
    login_prompt();
}