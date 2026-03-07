#include <stddef.h>
#include <stdint.h>

#include "libimagine.h"

// Copia n bytes de um local para outro. 
// Essencial para o realloc da Lua.
void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Preenche um bloco de memória com um valor.
// Usado para limpar estruturas de dados.
void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}