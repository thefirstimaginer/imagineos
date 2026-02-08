// Busca substring (strstr)
#include <stddef.h>
char* string_strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

// Função para ler caractere do shell (simula getchar)
char shell_getchar() {
    // Você pode implementar usando um buffer global ou integração com o kernel
    // Aqui retorna 0 (stub), mas pode ser substituído por leitura real
    return 0;
}

#include "print.h"
#include "string.h"
#include <stddef.h>
// Não usar stdio.h, usar apenas funções da biblioteca

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
        print_str("\n[LI]ERROR: Block between braces not found or incomplete!\n");
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
    // Prototipos ausentes
    int string_to_int(char* str, int* next_pos);
    void int_to_string(int value, char* out);

    char line[256];
    strncpy(line, code, 255);
    line[255] = 0;
    char* s = line;
    char token[32];
    char ask_buffer[32] = {0};
    while (*s) {
        s = next_token(s, token);
        if (strcmp(token, "put") == 0) {
            // put valor -> varname
            char valstr[16], arrow[8], varname[16];
            s = next_token(s, valstr);
            s = next_token(s, arrow);
            s = next_token(s, varname);
            if (strcmp(arrow, "->") == 0) {
                int val = string_to_int(valstr, NULL);
                create_var(varname, val);
            }
        } else if (strcmp(token, "ask") == 0) {
            // ask -> varname
            char arrow[8], varname[16];
            s = next_token(s, arrow);
            s = next_token(s, varname);
            if (strcmp(arrow, "->") == 0) {
                print_str("\nDigite valor: ");
                // Simula scanf: lê do teclado até Enter
                int i = 0;
                char c = 0;
                while (i < 31 && (c = shell_getchar()) != '\n' && c != '\r') {
                    if (c >= ' ' && c <= '~') {
                        ask_buffer[i++] = c;
                        char s[2] = {c, 0};
                        print_str(s);
                    }
                }
                ask_buffer[i] = 0;
                int val = string_to_int(ask_buffer, NULL);
                create_var(varname, val);
            }
        } else if (strcmp(token, "if") == 0) {
            // if var ==> valor ( ... ) else ( ... ) !endif
            char varname[16], op[8], valstr[16];
            s = next_token(s, varname);
            s = next_token(s, op);
            s = next_token(s, valstr);
            struct Var* v = find_var(varname);
            int cond = 0;
            if (v && strcmp(op, "==>") == 0) {
                int cmp = string_to_int(valstr, NULL);
                cond = (v->value == cmp);
            }
            // Pega bloco if (...)
            char* ifblk = strchr(s, '(');
            char* ifend = ifblk;
            int depth = 1;
            if (ifblk) ifblk++;
            while (ifend && depth > 0) {
                if (*ifend == '(') depth++;
                else if (*ifend == ')') depth--;
                if (depth > 0) ifend++;
            }
            char* elseblk = string_strstr(ifend, "else");
            char* elsebody = NULL;
            char* endif = string_strstr(ifend, "!endif");
            if (elseblk && endif) {
                elsebody = strchr(elseblk, '(');
                if (elsebody) elsebody++;
            }
            if (cond && ifblk && ifend) {
                char body[256];
                int len = ifend - ifblk;
                if (len > 255) len = 255;
                strncpy(body, ifblk, len);
                body[len] = 0;
                liteinterp_run(body);
            } else if (!cond && elsebody && endif) {
                char body[256];
                char* elseend = elsebody;
                int depth2 = 1;
                while (elseend && depth2 > 0) {
                    if (*elseend == '(') depth2++;
                    else if (*elseend == ')') depth2--;
                    if (depth2 > 0) elseend++;
                }
                int len = elseend - elsebody;
                if (len > 255) len = 255;
                strncpy(body, elsebody, len);
                body[len] = 0;
                liteinterp_run(body);
            }
            if (endif) s = endif + 6; // pula !endif
            else break;
        } else if (strcmp(token, "def") == 0) {
            // def nomeFunc (...)
            char fname[16];
            s = next_token(s, fname);
            // Pega o corpo até !enddef
            char* body_start = strchr(s, '(');
            if (body_start) body_start++;
            char* body_end = string_strstr(body_start, "!enddef");
            if (body_end && func_count < MAX_FUNCS) {
                int len = body_end - body_start;
                if (len > MAX_FUNC_BODY-1) len = MAX_FUNC_BODY-1;
                strncpy(funcs[func_count].name, fname, MAX_VAR_NAME-1);
                strncpy(funcs[func_count].body, body_start, len);
                funcs[func_count].body[len] = 0;
                func_count++;
                s = body_end + 7; // pula !enddef
            } else {
                print_str("\nErro: !enddef não encontrado ou limite de funções atingido!\n");
                break;
            }
        } else {
            // Execução normal (mantém comandos já existentes)
            if (strcmp(token, "var") == 0) {
                char name[16];
                s = next_token(s, name);
                while (*s && *s != '=') s++;
                if (*s == '=') s++;
                int value = string_to_int(s, NULL);
                create_var(name, value);
            } else if (strcmp(token, "sum") == 0) {
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
                // Tenta executar função definida
                int found = 0;
                for (int i = 0; i < func_count; i++) {
                    if (strcmp(token, funcs[i].name) == 0) {
                        liteinterp_run(funcs[i].body);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    while (*s && *s != ';') s++;
                    if (*s) s++;
                }
            }
        }
    }
}

void liteinterp_init() {
    return 0;
}
