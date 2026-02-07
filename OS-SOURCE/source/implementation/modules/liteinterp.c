#include "print.h"
#include "string.h"
#include <stddef.h>

#define MAX_VARS 16
#define MAX_VAR_NAME 16
#define MAX_FUNCS 8
#define MAX_FUNC_BODY 256

// Variável simples
struct Var {
    char name[MAX_VAR_NAME];
    int value;
};

static struct Var vars[MAX_VARS];
static int var_count = 0;

// Função definida pelo usuário
struct UserFunc {
    char name[MAX_VAR_NAME];
    char body[MAX_FUNC_BODY];
};
static struct UserFunc funcs[MAX_FUNCS];
static int func_count = 0;

// Busca variável
static struct Var* find_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return &vars[i];
    }
    return NULL;
}

// Cria variável
static struct Var* create_var(const char* name, int value) {
    struct Var* v = find_var(name);
    if (v) { v->value = value; return v; }
    if (var_count < MAX_VARS) {
        strncpy(vars[var_count].name, name, MAX_VAR_NAME-1);
        vars[var_count].value = value;
        var_count++;
        return &vars[var_count-1];
    }
    return NULL;
}

// Interpreta comandos
void liteinterp_run(const char* code);

void liteinterp(char* args) {
    // Reset vars/funcs para cada execução
    var_count = 0;
    func_count = 0;
    // Executa apenas o que está entre chaves
    char* start = args;
    while (*start && *start != '{') start++;
    if (*start == '{') start++;
    char* end = start;
    int depth = 1;
    while (*end && depth > 0) {
        if (*end == '{') depth++;
        else if (*end == '}') depth--;
        if (depth > 0) end++;
    }
    if (depth == 0) {
        char code[512];
        int len = end - start;
        if (len > 511) len = 511;
        strncpy(code, start, len);
        code[len] = 0;
        liteinterp_run(code);
    } else {
        print_str("\nErro: bloco entre chaves não encontrado ou incompleto!\n");
    }
}

// Função auxiliar para ler próximo token
static char* next_token(char* s, char* token) {
    while (*s == ' ' || *s == ';') s++;
    int i = 0;
    while (*s && *s != ' ' && *s != ';' && *s != '(' && *s != ')' && *s != '=' && *s != '>' && *s != '\0') {
        token[i++] = *s++;
    }
    token[i] = 0;
    return s;
}

// Interpretação principal (simplificada)
void liteinterp_run(const char* code) {
    char line[256];
    strncpy(line, code, 255);
    line[255] = 0;
    char* s = line;
    char token[32];
    while (*s) {
        s = next_token(s, token);
        if (strcmp(token, "var") == 0) {
            // var nome = valor;
            char name[16];
            s = next_token(s, name);
            while (*s && *s != '=') s++;
            if (*s == '=') s++;
            int value = string_to_int(s, NULL);
            create_var(name, value);
        } else if (strcmp(token, "sum") == 0) {
            // sum a b
            char a[16], b[16];
            s = next_token(s, a);
            s = next_token(s, b);
            struct Var* va = find_var(a);
            struct Var* vb = find_var(b);
            if (va && vb) {
                print_str("\n");
                int_to_string(va->value + vb->value, a);
                print_str(a);
            }
        } else if (strcmp(token, "min") == 0) {
            char a[16], b[16];
            s = next_token(s, a);
            s = next_token(s, b);
            struct Var* va = find_var(a);
            struct Var* vb = find_var(b);
            if (va && vb) {
                print_str("\n");
                int_to_string(va->value - vb->value, a);
                print_str(a);
            }
        } else if (strcmp(token, "mult") == 0) {
            char a[16], b[16];
            s = next_token(s, a);
            s = next_token(s, b);
            struct Var* va = find_var(a);
            struct Var* vb = find_var(b);
            if (va && vb) {
                print_str("\n");
                int_to_string(va->value * vb->value, a);
                print_str(a);
            }
        } else if (strcmp(token, "div") == 0) {
            char a[16], b[16];
            s = next_token(s, a);
            s = next_token(s, b);
            struct Var* va = find_var(a);
            struct Var* vb = find_var(b);
            if (va && vb && vb->value != 0) {
                print_str("\n");
                int_to_string(va->value / vb->value, a);
                print_str(a);
            }
        } else if (strcmp(token, "say") == 0) {
            // say "mensagem"
            while (*s == ' ' || *s == '"') s++;
            char* msg = s;
            while (*s && *s != '"' && *s != ';') s++;
            char old = *s;
            *s = 0;
            print_str("\n");
            print_str(msg);
            *s = old;
            if (*s) s++;
        } else {
            // Ignora tokens desconhecidos por enquanto
            while (*s && *s != ';') s++;
            if (*s) s++;
        }
    }
}

void liteinterp_init() {
    print_str("\nLiteInterp pronto!\n");
}
