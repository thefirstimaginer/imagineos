#include "print.h"
#include "string.h"

// Protótipos das funções de sistema
void shutdown_system(void);

void halt_init() {
    // Inicializar o módulo (opcional)
    print_str("");
}

void halt_run(char* args) {
    // Se houver argumentos, pode mostrar ajuda
    if (strcmp(args, "help") == 0 || strcmp(args, "ver") == 0) {
        print_str("\nHalt Module v1.0 - ImagineOS\n");
        print_str("Copyright (C) 2024-2026, ImagineOS Project\n");
        print_str("\nUsage: halt\n");
        print_str("This command will shutdown the system.\n");
        return;
    }
    
    // Executar shutdown
    print_str("\nShutting down...\n");
    shutdown_system();
    
    // Se por algum motivo não desligar, fazer halt
    for(;;) { asm volatile("hlt"); }
}