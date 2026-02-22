//Calculator
#include "print.h"
#include "string.h"

void calc_init() {
    print_str("");
}

void calc_run(char* args) {
    calc_module_main(args);
}

void calc_module_main(char* args)
{
    // Ajuda
    if (strcmp(args, "ver") == 0 || strcmp(args, "help") == 0) {
        print_str("Calculator Module v2.0 - ImagineOS\n");
        print_str("Copyright (C) 2024-2026, ImagineOS Project\n");
        print_str("\nUsage: calc <num1> <op> <num2>\n");
        print_str("\nOperators: +, -, *, /");
        return;
    }

    // Ignora espaços
    char* input = skip_spaces(args);
    int erro = 0;
    int resultado = 0;

    // Extrai primeiro número
    int pos = 0;
    int n1 = string_to_int(input, &pos);
    input += pos;
    input = skip_spaces(input);

    // Extrai operador
    char operator = *input;
    if (operator != '+' && operator != '-' && operator != '*' && operator != '/') {
        print_str("[ERROR]: Unknown operator. Use +, -, * or /");
        return;
    }
    input++;
    input = skip_spaces(input);

    // Extrai segundo número
    int n2 = string_to_int(input, NULL);

    // Lógica de cálculo
    if (operator == '+') resultado = n1 + n2;
    else if (operator == '-') resultado = n1 - n2;
    else if (operator == '*') resultado = n1 * n2;
    else if (operator == '/') {
        if (n2 != 0) resultado = n1 / n2;
        else {
            print_str("[ERROR]: Division by zero!");
            erro = 1;
        }
    }

    if (!erro) {
        char res_str[32];
        int_to_string(resultado, res_str);
        print_str("[RESULT]: ");
        print_str(res_str);
    }
}