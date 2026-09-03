#include "init.h"
#include "process.h"
#include "modules.h"
#include "shell.h"

extern void login_prompt(void);
extern void calc_run(char* args);
extern void liteinterp(char* args);
extern void halt_run(char* args);
extern void ver_run(char* args);
extern void help_run(char* args);
extern void clear_run(char* args);
extern void video_run(void);
extern void ps_run(char* args);

void init_system(void) {
    process_init();
    process_create_named("shell", (void (*)(void))shell_run);
    process_create_named("calc", (void (*)(void))calc_run);
    process_create_named("liteinterp", (void (*)(void))liteinterp);
    process_create_named("halt", (void (*)(void))halt_run);
    process_create_named("ver", (void (*)(void))ver_run);
    process_create_named("help", (void (*)(void))help_run);
    process_create_named("clear", (void (*)(void))clear_run);
    process_create_named("video", (void (*)(void))video_run);
    process_create_named("ps", (void (*)(void))ps_run);
    modules_load();
    login_prompt();
}