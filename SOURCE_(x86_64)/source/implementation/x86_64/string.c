#include "sys_includes/string.h"
#include <stddef.h>

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Função auxiliar para inverter a string (necessária para o itoa)
void reverse(char* s) {
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

// Converte inteiro para string
void int_to_string(int n, char* str) {
    int i = 0;
    int is_negative = 0;

    // Trata o zero explicitamente
    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Trata números negativos
    if (n < 0) {
        is_negative = 1;
        n = -n;
    }

    // Extrai os dígitos de trás para frente
    while (n != 0) {
        str[i++] = (n % 10) + '0';
        n = n / 10;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse(str); // Inverte para a ordem correta
}

// Retorna o tamanho da string
size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

// Encontra a primeira ocorrência de um caractere
char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s) return 0;
        s++;
    }
    return (char*)s;
}

// Copia n caracteres de uma string para outra
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < n; i++)
        dest[i] = '\0';
    return dest;
}
