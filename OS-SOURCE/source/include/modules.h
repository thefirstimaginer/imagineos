#pragma once

typedef struct Module {
	const char* name;
	void (*init)();
	void (*run)(char* args);
} Module;

extern Module modules[];
extern int modules_count;
void modules_load();
void calc_init();
void calc_run(char* args);

void modules_init();
void modules_output();