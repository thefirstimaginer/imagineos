#include "print.h"
#include "shell.h"
#include <x86_64/rtc.h>

#include "libraries/string.h"
#include "libraries/math.h"
#include "modules.h"

char last_command[128] = {0};// Buffer para armazenar o último comando digitado

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
    print_str("                       |___/                   Versão Alpha\n");
    print_str("  __________________________________________________________  \n\n");
    
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    print_str(" Build: 20251231 | Prévia de Desenvolvimento | Kernel: x86_64\n");
    print_str(" Digite 'help' para ver os comandos.\n\n");
    
    shell_print_prompt();
}

// Variáveis para armazenar a posição do prompt do shell
void shell_print_prompt() {                             // print shell prompt and set editable area
    // Imprime o prompt e só então registra o início da área editável
    print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK); // cor verde para o prompt
    print_str("shell@: ");                              // imprime o prompt
    shell_prompt_col = col;                             // registra a coluna do prompt
    shell_prompt_row = row;                             // registra a linha do prompt
    enable_cursor(14, 15);                              // cursor block
    set_cursor(col, row);                               // garante que o cursor está na posição correta
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
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
    
    // Handle empty command
    if (cmd_name[0] == '\0') {
        print_char('\n');
        shell_print_prompt();
        return; // Sai da função sem passar pelos outros else if
    }

    // If the command is empty, just print a new prompt
    if (is_empty(cmd_name)) {
    print_char('\n');
    shell_print_prompt();
    return;
    }

    // Handle empty command
    if (cmd_name[0] == '\0') {
        print_char('\n');
        shell_print_prompt();
        return;
    }

    // GUARDA NO HISTÓRICO: Antes de processar, copiamos para o last_command
    strncpy(last_command, cmd, 127);

    // handle commands (compare manually to avoid relying on <string.h>)
    //Help
    if (strcmp(cmd, "help") == 0)
    {
        print_str("\n");
        print_str("+---------------------------------------------+\n");
        print_str("|");
        print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
        print_str("        GUIA DE AJUDA DO IMAGINE OS v1.0.2       ");
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        print_str("|\n");
        print_str("+-----------------------------------------------+\n");
        print_str("|            [COMANDOS DO SISTEMA]              |\n");
        print_str("| [help] - Mostra essa tela.                    |\n");
        print_str("| [date] - Mostra a data e hora atual.          |\n");
        print_str("| [clear] - Limpa o Shell.                      |\n");
        print_str("| [history] - Histórico de comandos, mostra     |\n");
        print_str("| somente o último comando                      |\n");
        print_str("| [ver] - Mostra a versão do Sistema.           |\n");
        print_str("| [reboot] - Reinicia o Sistema.                |\n");
        print_str("| [exit] - Encerra o Sistema.                   |\n");
        print_str("| [(PROG) ver] - Mostra a versão do programa.   |\n");
        print_str("| [(PROG) help] - Mostra a ajuda do programa.   |\n");
        print_str("|                                               |\n");
        print_str("|                 [DESTAQUES]                   |\n");
        print_str("| [calc] - Operações Básicas (2 NÚMEROS APENAS!)|\n");
        print_str("| [hello] - Hello World!                        |\n");
        //print_str("| [writer] - Hello World!                        |\n");
        print_str("+-----------------------------------------------+");
    }

    //Version
    else if(strcmp(cmd, "ver") == 0)
    {
        print_set_color(PRINT_COLOR_CYAN, PRINT_COLOR_BLACK);
        print_str("\n");
        print_str("  __________________________________________________________  \n");
        print_str("   ___                       _             ___  ____  \n");
        print_str("  |_ _|_ __ ___   __ _  __ _(_)_ __   ___ / _ \\/ ___| \n");
        print_str("   | || '_ ` _ \\ / _` |/ _` | | '_ \\ / _ \\ | | \\___ \\ \n");
        print_str("   | || | | | | | (_| | (_| | | | | |  __/ |_| |___) |\n");
        print_str("  |___|_| |_| |_|\\__,_|\\__, |_|_| |_|\\___|\\___/|____/ \n");
        print_str("                       |___/    Versão Alpha, Janeiro de 2026\n");
        print_str("           Copyright (C) 2024-2026, O Projeto ImagineOS\n");
        print_str("    Bugs? mande um e-mail para: <nyxieworlduniverse@gmail.com>\n");
        print_str("  __________________________________________________________  ");
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    }

    //History
    else if (strcmp(cmd_name, "history") == 0) {
        if (last_command[0] == '\0') {
            print_str("\nSem Histórico.");
        } else {
            print_str("\nÚltimo Comando: ");
            print_str(last_command);
        }
    }

    //Hello World!
    else if (strcmp(cmd, "hello") == 0)
    {
        print_str("\nHello World!");
    }

    //Call
    else if (strcmp(cmd_name, "call") == 0) {
        if (strcmp(args, ".err_screen") == 0)
        {
            print_clear();
            print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLUE);
            print_str("ERRO!, ISSO É UM ERRO!");
            color = PRINT_COLOR_WHITE | PRINT_COLOR_BLUE << 4;
        }
    }

    //Clear
    else if (strcmp(cmd, "clear") == 0)
    {
        print_clear();
        row = 0;
        col = 0;
        shell_print_prompt();
        return;
    }

    //Reboot
    else if (strcmp(cmd, "reboot") == 0)
    {
        print_str("Reiniciando...\n");              // feedback
        reboot_system();                            // reboot system
        // if reboot fails, loop
        for(;;);                                    // hang
    }

    //Shutdown
    else if (strcmp(cmd, "exit") == 0)
    {
        print_str("Shutting down...\n");
        shutdown_system();
        for(;;); // hang if shutdown fails
    }

    //Date command
    else if (strcmp(cmd, "date") == 0)
    {
        print_char('\n');
        
        // Lê os dados do RTC usando a lógica que você já tinha
        uint8_t day    = rtc_get_value(RTC_REGISTER_DAY);
        uint8_t month  = rtc_get_value(RTC_REGISTER_MONTH);
        uint8_t year   = rtc_get_value(RTC_REGISTER_YEAR);
        uint8_t hour   = rtc_get_value(RTC_REGISTER_HOURS);
        uint8_t minute = rtc_get_value(RTC_REGISTER_MINUTES);

        print_str("[DATA]: ");
        print_uint64_dec(day);
        print_char('/');
        print_uint64_dec(month);
        print_str("/20"); // O RTC costuma dar o ano com 2 dígitos (ex: 25)
        print_uint64_dec(year);

        print_str("  [HORA]: ");
        if (hour < 10) print_char('0');
        print_uint64_dec(hour);
        print_char(':');
        if (minute < 10) print_char('0');
        print_uint64_dec(minute);
        
        print_char('\n');
        shell_print_prompt();
        return;
    }

    //Calculator
    else if(strcmp(cmd, "writer") ==0) {

        print_str("\nImagine(R) Writer v0.1");
        print_clear();
        return;
    }
    else if(strcmp(cmd, "diskmate") ==0) {

        print_str("\nImagine(R) DISKMATE v0.1");
        print_str("\nUNDER CONSTRUCTION");
    }
    else if (strcmp(cmd_name, "calc") == 0) {
        init_calc();
    }
    else {
        print_str("\n[SYS]: Comando Desconhecido: '");
        print_str(cmd_name);
        print_str("'");
    }
    print_char('\n');                                   // move to next line
    shell_print_prompt();                               // print new prompt
}