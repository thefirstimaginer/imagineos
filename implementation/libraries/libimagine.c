// LibImagine is the standard C library for ImagineOS
#include "libimagine.h"
#include <stdint.h>
#include <stddef.h>

// Reserva 1MB para a Lua e o sistema trabalharem
#define HEAP_SIZE 1024 * 1024
static uint8_t heap[HEAP_SIZE];
static size_t next_free = 0;

void* malloc(size_t size) {
    // Alinha para 8 bytes (importante para 64-bit no VirtualBox)
    size = (size + 7) & ~7;

    if (next_free + size > HEAP_SIZE) {
        return NULL; // Acabou a memória
    }

    void* ptr = &heap[next_free];
    next_free += size;
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        // No bump allocator, não liberamos memória, 
        // então apenas ignoramos o free por enquanto.
        return NULL; 
    }

    void* new_ptr = malloc(size);
    if (new_ptr) {
        // Nota: Em um realloc real, saberíamos o tamanho antigo.
        // Como não sabemos aqui, copiamos o máximo possível (size).
        // Isso é perigoso, mas funcional para testes iniciais.
        memcpy(new_ptr, ptr, size); 
    }
    return new_ptr;
}

void free(void* ptr) {
    // Em um Bump Allocator, o free não faz nada. 
    // A memória só é limpa quando o sistema reinicia.
}