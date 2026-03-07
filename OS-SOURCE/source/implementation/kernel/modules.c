#include "modules.h"
#include "print.h"


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
        return;
    }
}

void modules_load() {
    modules_init();
    modules_output();
}

// ...existing code...
