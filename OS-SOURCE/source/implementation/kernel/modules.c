#include "modules.h"
#include "print.h"
#include "modules.h"
#include "print.h"

// Prototipos dos módulos
void calc_init();
void calc_run(char* args);

// Lista de módulos
Module modules[] = {
    {"calc", calc_init, calc_run},
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
    print_str("\n");
    for (int i = 0; i < sizeof(modules)/sizeof(Module); i++) {
        print_str("");
    }
}

void modules_load() {
    modules_init();
    modules_output();
}

// ...existing code...
