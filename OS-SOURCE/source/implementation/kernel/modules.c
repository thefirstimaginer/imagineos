#include "modules.h"
#include "print.h"
#include "modules.h"
#include "print.h"

// Prototipos dos módulos
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

// Lista de módulos
Module modules[] = {
    {"calc", calc_init, calc_run},
    {"liteinterp", liteinterp_init, liteinterp},
    {"li", liteinterp_init, liteinterp},
    {"halt", halt_init, halt_run},
    {"ver", ver_init, ver_run},
    {"help", help_init, help_run},
    // Adicione outros módulos aqui
};

int modules_count = sizeof(modules)/sizeof(Module);

int modules_loaded = 0;

void modules_init() {
    for (int i = 0; i < sizeof(modules)/sizeof(Module); i++) {
        if (modules[i].init) modules[i].init();
    }
    modules_loaded = 1;
// ...existing code...
}

void modules_output() {
    for (int i = 0; i < sizeof(modules)/sizeof(Module); i++) {
        return 0;
    }
}

void modules_load() {
    modules_init();
    modules_output();
}

// ...existing code...
