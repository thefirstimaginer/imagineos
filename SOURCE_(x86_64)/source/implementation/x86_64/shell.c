#include "print.h"
#include "shell.h"
#include "x86_64/rtc.h"

#include "sys_includes/string.h"
#include "sys_includes/math.h"

// Note que agora as variáveis 'buffer', 'color', 'col', 'row' 
// precisam ser acessadas. Como elas estão no print.c, 
// usamos 'extern' para avisar o compilador.
extern uint8_t color;
extern size_t col;
extern size_t row;

extern struct Char* buffer;
struct Char {uint8_t character; uint8_t color; };
extern size_t NUM_COLS;
extern size_t NUM_ROWS;

extern size_t shell_prompt_col; // Variável para armazenar a coluna do prompt do shell
extern size_t shell_prompt_row;

void shell_init() {
    print_clear(); // Limpa qualquer mensagem de debug do boot
    row = 0;
    col = 0;
    
    print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK); // Uma cor diferente para o título
print_str("  __________________________________________________________  \n");
    print_str("   ___                       _             ___  ____  \n");
    print_str("  |_ _|_ __ ___   __ _  __ _(_)_ __   ___ / _ \\/ ___| \n");
    print_str("   | || '_ ` _ \\ / _` |/ _` | | '_ \\ / _ \\ | | \\___ \\ \n");
    print_str("   | || | | | | | (_| | (_| | | | | |  __/ |_| |___) |\n");
    print_str("  |___|_| |_| |_|\\__,_|\\__, |_|_| |_|\\___|\\___/|____/ \n");
    print_str("                       |___/                           \n");
    print_str("  __________________________________________________________  \n\n");
    
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    print_str(" Build: 20251228 | Dev Preview | Kernel: x86_64\n");
    print_str(" Type 'help' to see available commands.\n\n");
    
    shell_print_prompt();
}

// Variáveis para armazenar a posição do prompt do shell
void shell_print_prompt() {                             // print shell prompt and set editable area
    // Imprime o prompt e só então registra o início da área editável
    //print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK); // cor verde para o prompt
    print_str("shell:$ ");                              // imprime o prompt
    shell_prompt_col = col;                             // registra a coluna do prompt
    shell_prompt_row = row;                             // registra a linha do prompt
    enable_cursor(14, 15);                              // cursor block
    set_cursor(col, row);                               // garante que o cursor está na posição correta
}

// Função para processar o comando digitado no prompt
void shell_handle_enter(void) {                             // process command entered at prompt
    // Read the typed command from the screen buffer (from prompt start to current cursor)
    size_t start = shell_prompt_row * NUM_COLS + 
    shell_prompt_col;                                   // start of editable area
    size_t end = row * NUM_COLS + col;                  // current cursor position
    size_t len = 0;                                     // length of command

    if (end > start) len = end - start;                 // compute length
    if (len > 127) len = 127;                           // limit length to buffer size

    char cmd[128];                                      // command buffer (+1 for null terminator)
    for (size_t i = 0; i < len; i++) {                  // copy characters
        cmd[i] = (char) buffer[start + i].character;    // copy character
    }
    cmd[len] = '\0';// null-terminate string

    // Function to convert string to integer
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

    // Parse command and arguments
    char cmd_name[32] = {0};
    char* args = "";
    
    // Procura o primeiro espaço
    char* space = strchr(cmd, ' ');
    
    if (space) {
        // Se tem espaço, o comando é o que vem antes
        strncpy(cmd_name, cmd, space - cmd);
        args = space + 1; // Argumentos começam depois do espaço
    } else {
        // Se não tem espaço, o comando é a string inteira
        strncpy(cmd_name, cmd, 31);
    }

    // handle commands (compare manually to avoid relying on <string.h>)

    if (strcmp(cmd, "help") == 0)
    {
        print_str("\n");
        print_str("+--------------------------------------------+\n");
        print_str("|         ImagineOS Help Guide V1.0          |\n");
        print_str("+--------------------------------------------+\n");
        print_str("| [help] - Display this screen.              |\n");
        print_str("| [hello] - Say hello!                       |\n");
        print_str("| [reboot] - Reboot the System.              |\n");
        print_str("| [clear] - Clear the Display.               |\n");
        print_str("| [ver] - Show the current version.          |\n");
        print_str("| [date] - Show the current date.            |\n");
        print_str("| [exit] - Turn off the system.              |\n");
        print_str("| [listapp] - ListApp Program List.          |\n");
        print_str("| [(PROG) ver] - Show the program version.   |\n");
        print_str("| [(PROG) help] - Show the program help.     |\n");
        print_str("+--------------------------------------------+");
    }
    else if (strcmp(cmd, "clear") == 0)
    {
        print_clear();
        row = 0;
        col = 0;
        shell_print_prompt();
        return;
    }
    else if (strcmp(cmd, "date") == 0)
    {
        print_char('\n');
        
        // Lê os dados do RTC usando a lógica que você já tinha
        uint8_t day    = rtc_get_value(RTC_REGISTER_DAY);
        uint8_t month  = rtc_get_value(RTC_REGISTER_MONTH);
        uint8_t year   = rtc_get_value(RTC_REGISTER_YEAR);
        uint8_t hour   = rtc_get_value(RTC_REGISTER_HOURS);
        uint8_t minute = rtc_get_value(RTC_REGISTER_MINUTES);

        print_str("Current Date: ");
        print_uint64_dec(day);
        print_char('/');
        print_uint64_dec(month);
        print_str("/20"); // O RTC costuma dar o ano com 2 dígitos (ex: 25)
        print_uint64_dec(year);

        print_str("  Hour: ");
        if (hour < 10) print_char('0');
        print_uint64_dec(hour);
        print_char(':');
        if (minute < 10) print_char('0');
        print_uint64_dec(minute);
        
        print_char('\n');
        shell_print_prompt();
        return;
    }
    else if(strcmp(cmd, "ver") == 0)
    {
        print_str("\n");
        print_str("ImagineOS Alpha - Development Preview\n");
        print_str("Copyright (C) 2025 The Imagine OS Project\n");
        print_str("Bugs? send a mail to: <nyxieworlduniverse@gmail.com>");
    }
    else if (strcmp(cmd, "hello") == 0)
    {
        print_str("\nHello World!");
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        print_str("Rebooting...\n");                // feedback
        reboot_system();                            // reboot system
        // if reboot fails, loop
        for(;;);                                    // hang
    }
    else if (strcmp(cmd, "exit") == 0)
    {
        print_str("Shutting down...\n");
        shutdown_system();
        for(;;); // hang if shutdown fails
    }
    else if (strcmp(cmd, "listapp") == 0)
    {
        print_str("\n");
        print_str("+----------------------------------------+\n");
        print_str("|         ImagineOS Applications         |\n");
        print_str("+----------------------------------------+\n");
        print_str("| [calc] - Imagine Calculator            |\n");
        print_str("| [listapp ver]  - Show listapp version  |\n");
        print_str("+----------------------------------------+\n");
    }
    else if (strcmp(cmd_name, "calc") == 0) {
        int pos = 0;
        
        // 1. Converte o primeiro número
        int n1 = string_to_int(args, &pos);
        
        // 2. O operador está na posição onde o string_to_int parou
        char op = args[pos];
        
        // 3. Converte o segundo número (pulando o operador)
        int n2 = string_to_int(&args[pos + 1], NULL);

        int resultado = 0;
        int erro = 0;

        // 4. Lógica de cálculo
        if (op == '+') resultado = n1 + n2;
        else if (op == '-') resultado = n1 - n2;
        else if (op == '*') resultado = n1 * n2;
        else if (op == '/') {
            if (n2 != 0) resultado = n1 / n2;
            else {
                print_str("\nError: Division by zero!");
                erro = 1;
            }
        } else {
            print_str("\nError: Unknown operator. Use +, -, * or /");
            erro = 1;
        }

        if (!erro) {
            char res_str[32];
            int_to_string(resultado, res_str); // Usa sua nova biblioteca!
            
            print_str("\nResult: ");
            print_str(res_str);
        }
    }
    else {
        print_str("\nComando desconhecido: ");
        print_str(cmd_name);
    }

    /*
    //listapp command
    if (cmd[0] == 'l' && cmd[1] == 'i' && cmd[2] == 's' && cmd[3] == 't' && cmd[4] == 'a' && cmd[5] == 'p' && cmd[6] == 'p') {
        char* args = &cmd[8]; // Pula "calc "


        if (args[0] == ' ' || args[1] == ' ') {
            print_str("\nUSAGE: listapp (command)!");
        }

        if (args[0] == 'v' && args[1] == 'e' && args[2] == 'r') {
            print_str("\nImagineOS ListApp V1.0");
        }

        if (strcmp(cmd, "help") == 0) {
            print_str("\n");
            print_str("+----------------------------------------+\n");
            print_str("|      ImagineOS ListApp Help Guide      |\n");
            print_str("+----------------------------------------+\n");
            print_str("| [listapp apps] - Show all listapp apps |\n");
            print_str("| [listapp ver]  - Show listapp version  |\n");
            print_str("+----------------------------------------+\n");
        }

        if (args[0] == 'a' && args[1] == 'p' && args[2] == 'p' && args[3] == 's') {
            print_str("\n");
            print_str("+----------------------------------------+\n");
            print_str("|         ImagineOS Applications         |\n");
            print_str("+----------------------------------------+\n");
            print_str("| [calc] - Imagine Calculator            |\n");
            print_str("| [listapp ver]  - Show listapp version  |\n");
            print_str("+----------------------------------------+\n");
        }

        int pos = 0;
    }


    // Comando da Calculadora
    if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 'l' && cmd[3] == 'c') {
        char* args = &cmd[5]; // Pula "calc "

        if (args[0] == 'v' && args[1] == 'e' && args[2] == 'r') {
            print_str("\nImagineOS Calculator V1.0");
            goto end_calc;
        }

        int pos = 0;

        // 1. Pega o primeiro número
        int n1 = string_to_int(args, &pos);
        
        // 2. O sinal está logo após o primeiro número
        char op = args[pos];
        
        // 3. Pega o segundo número (começando 1 posição após o sinal)
        int pos2 = 0;
        int n2 = string_to_int(&args[pos + 1], &pos2);

        int resultado = 0;

        // 4. Decide a conta baseada no sinal
        if (op == '+') resultado = n1 + n2;
        else if (op == '-') resultado = n1 - n2;
        else if (op == '*') resultado = n1 * n2;
        else if (op == '/') {
            if (n2 != 0) resultado = n1 / n2;
            else {
                print_str("\nErro: Divisao por zero!");
                goto end_calc;
            }
        }

        // 5. Exibe o resultado usando sua função de imprimir números
        print_str("\nResult: ");
        print_uint64_dec(resultado);

    end_calc:
        print_char('\n');
        shell_print_prompt();
        return;
    }

    // clear command
    if (len == 5) {                                     // length 5
        const char want_clear[5] = {'c','l','e','a','r'};     // clear
        int match_clear = 1;                                  // assume match
        for (size_t i = 0; i < 5; i++) {                // compare each char
            if (cmd[i] != want_clear[i]) { 
                match_clear = 0; break; 
            }// no match
        }
        if (match_clear) {                                    // if matched
            // Clear whole screen and reset cursor
            print_clear();                              // clear screen
            row = 0;                                    // reset row
            col = 0;                                    // reset col
            shell_print_prompt();                       // print new prompt
            return;                                     // done
        }
    }

    // reboot command
    if (len == 6) {                                     // length 6
        const char want_reboot[6] = {'r','e','b','o','o','t'};// reboot
        int match_reboot = 1;                                 // assume match
        for (size_t i = 0; i < 6; i++) {                      // compare each char
            if (cmd[i] != want_reboot[i]) {                   // no match
                match_reboot = 0; break;                      // no match
            }
        }
        if (match_reboot) {                             // if matched
            print_str("Rebooting...\n");                // feedback
            reboot_system();                            // reboot system
            // if reboot fails, loop
            for(;;);                                    // hang
        }
    }

    // Version Command
    if (len == 3){
        const char want_ver[3] = {'v','e','r'};
        int match_ver = 1;
        for (size_t i = 0; i < 3; i++) {
            if (cmd[i] != want_ver[i]){
                match_ver = 0; break;
            }
        }
        if (match_ver){
            print_str("\n");
            print_str("ImagineOS Alpha - Development Preview\n");
            print_str("Copyright (C) 2025 The Imagine OS Project\n");
            print_str("Developed by: Adryan Alcantara <thenyxiecreator@yahoo.com>\n\n");
            print_set_color(PRINT_COLOR_BLACK, PRINT_COLOR_YELLOW);
            print_str("UNDER CONSTRUCTION!");
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_str("\n");
        }
    }

    // shutdown command
    if (len == 4) {
        const char want_shutdown[4] = {'e','x','i','t'};
        int match_shutdown = 1;// assume match
        for (size_t i = 0; i < 4; i++ // compare each char) {
            if (cmd[i] != want_shutdown[i]) {
                match_shutdown = 0; break;
            }
        }
        if (match_shutdown){
            print_str("Shutting down...\n");
            shutdown_system();
            for(;;); // hang if shutdown fails
        }
    }

    // hello command
    if (len == 5){
        const char want_hello[5] = {'h','e','l','l','o'};
        int match_hello = 1;
        for (size_t i = 0; i < 5; i++) {
            if (cmd[i] != want_hello[i]){
                match_hello = 0;
                break;
            }
        }
        if (match_hello){
            print_str("\nHello World!");
        }
    }

    // main (works like sudo) command
    if (len == 4){
        const char want_main[4] = {'m','a','i','n'};
        int match_main = 1;
        for (size_t i = 0; i < 4; i++) {
            if (cmd[i] != want_main[i]){
                match_main = 0;
                break;
            }
        }
        if (match_main){
            print_str("\n");
            print_str("ImagineOS Maintener Alpha 1.0\n");
            print_set_color(PRINT_COLOR_BLACK, PRINT_COLOR_YELLOW);
            print_str("This feature still not completed yet!");
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            print_str("\n");
        }
    }

    //color command
    if (cmd[0] == 'c' && cmd[1] == 'o' && cmd[2] == 'l' && cmd[3] == 'o' && cmd[4] == 'r') {
        char* args = &cmd[6]; 
        int pos = 0;
        int new_color = string_to_int(args, &pos);

        if (new_color >= 0 && new_color <= 15){
            color = (uint8_t) new_color | (PRINT_COLOR_BLACK << 4); // Atualiza a variável de cor global
            // Aplicar a cor agora
            print_str("\nColor changed successfully.");
        } else {
            print_str("\nErr: Choose a color between 0 and 15!");
        }

        print_char('\n');
        shell_print_prompt();
        return;
    }

    //help command
    if (len == 4) {
        const char want_help[4] = {'h','e','l','p'};
        int match_help = 1;// assume match
        for (size_t i = 0; i < 4; i++// compare each char) {
            if (cmd[i] != want_help[i]) {
                match_help = 0; break;
            }
        }
        if (match_help){
            print_str("\n");
            print_str("+--------------------------------------------+\n");
            print_str("|         ImagineOS Help Guide V1.0          |\n");
            print_str("+--------------------------------------------+\n");
            print_str("| [help] - Display this screen.              |\n");
            print_str("| [hello] - Say hello!                       |\n");
            print_str("| [reboot] - Reboot the System.              |\n");
            print_str("| [clear] - Clear the Display.               |\n");
            print_str("| [ver] - Show the current version.          |\n");
            print_str("| [date] - Show the current date.            |\n");
            print_str("| [exit] - Turn off the system.              |\n");
            print_str("| [listapp] - ListApp Program List.          |\n");
            print_str("| [(PROG) ver] - Show the program version.   |\n");
            print_str("| [(PROG) help] - Show the program help.     |\n");
            print_str("+--------------------------------------------+");
        }
    }
    */

    // default behaviour: print newline and new prompt
    print_char('\n');                                   // move to next line
    shell_print_prompt();                               // print new prompt
}