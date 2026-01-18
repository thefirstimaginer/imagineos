#include "modules.h"
#include "libraries/math.h"
#include "libraries/string.h"
#include "libraries/libimagine.h"
#include "print.h"

extern  else if (strcmp(cmd_name, "calc") == 0);

void init_calc(void){

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