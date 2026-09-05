#include "modules.h"
#include "print.h"

/* protótipos do módulo de vídeo (definidos em modules/video.c) */
extern void video_init();
extern void video_run();
extern Module video_module;

// Lista de módulos
Module modules[] = {
    {"calc", calc_init, calc_run},
    {"liteinterp", liteinterp_init, liteinterp},
    {"li", liteinterp_init, liteinterp},
    {"ver", ver_init, ver_run},
    {"clear", clear_init, clear_run},
    {"ps", ps_init, ps_run},
    // Adicione outros módulos aqui
};

int modules_count = sizeof(modules)/sizeof(Module);

int modules_loaded = 0;

void modules_init() {
    for (int i = 0; i < modules_count; i++) {
        if (modules[i].init) modules[i].init();
    }
    modules_loaded = 1;
// ...existing code...
}

void modules_output() {
    for (int i = 0; i < modules_count; i++) {
        return;
    }
}

void modules_load() {
    modules_init();
    modules_output();
}

// ...existing code...
