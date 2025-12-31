#include "modules.h"
#include "print.h"
#include "shell.h"
#include "libraries/math.h"
#include "libraries/string.h"

extern char* args = "";

void init_calc(void){

    int string_to_int(char* str, int* next_pos) {
        int res = 0;
        int i = 0;

        // Pula espaços se houver
        while (str[i] == ' ') i++;

        // Converte os caracteres '0'-'9' em valor numérico
        while (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + (str[i] - '0');
            i++;
        }

        if (next_pos) *next_pos = i; 
        return res;
    }

    int pos = 0;
    
    // 1. Converte o primeiro número
    int n1 = string_to_int(args, &pos);
    
    // 2. O operador está na posição onde o string_to_int parou
    char operator = args[pos];
    
    // 3. Converte o segundo número (pulando o operador)
    int n2 = string_to_int(&args[pos + 1], NULL);

    int resultado = 0;
    int erro = 0;

    if (strcmp(args, "ver") == 0)
    {
    print_str("\nMódulo Calculadora v1.0 - ImagineOS\n");
    print_str("\nCopyright (C) 2024-2026, Projeto ImagineOS\n");
    }

    // 4. Lógica de cálculo
    if (operator == '+') resultado = n1 + n2;
    else if (operator == '-') resultado = n1 - n2;
    else if (operator == '*') resultado = n1 * n2;
    else if (operator == '/') {
        if (n2 != 0) resultado = n1 / n2;
        else {
            print_str("\nERRO: Divisão por ZERO!");
            erro = 1;
        }
    } else {
        print_str("\nERRO: Operador desconhecido!. Use [ +, -, * ou / ]");
        erro = 1;
    }

    if (!erro) {
        char res_str[32];
        int_to_string(resultado, res_str); // Usa sua nova biblioteca!
        
        print_str("\nResultado: ");
        print_str(res_str);
    }
}