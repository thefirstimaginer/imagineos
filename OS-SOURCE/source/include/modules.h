#pragma once

typedef struct Module {
	const char* name;
	void (*init)();
	void (*run)(char* args);
} Module;

extern Module modules[];
extern int modules_count;
// Registering Modules
void modules_load();
void calc_init();
void calc_run(char* args);
void liteinterp_init();
void liteinterp(char* args);
void halt_init();
void halt_run(char* args);
void ver_init();
void ver_run(char* args);
void help_init();
void help_run(char* args);

//Registering Functions
void calc_module_main();
void help_module_main();
void ver_module_main();

void modules_init();
void modules_output();