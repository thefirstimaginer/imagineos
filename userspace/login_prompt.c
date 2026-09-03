/* Imagine Operating System - vR1_0.1.0 - Codename "Jessica"
 * Copyright (c) 2026, Adryan Alcantara & TeamImagine
 * Licensed Under IGPLv1
 *
 * Login Prompt for ImagineOS.
 */

#include "print.h"
#include "userspace/login.h"
#include "libraries/libimagine.h"
#include "userspace/QBshell/tty.h"

// Buffer de entrada para o login
static char login_input_buffer[256] = {0};
static int login_input_index = 0;

extern void shell_add_char(char c);  // Função do shell para adicionar caracteres

// Estado da máquina de login
static int login_state = 0;  // 0 = username, 1 = hostname, 2 = done
static int shell_initialized = 0;  // Flag para inicializar shell apenas uma vez
static char temp_username[32] = {0};
static char temp_hostname[32] = {0};

void login_handle_input(char c) {
    if (c == '\n' || c == '\r') {
        // Processar a entrada
        login_input_buffer[login_input_index] = '\0';
        
        if (login_state == 0) {
            // Esperava username
            if (login_input_index > 0) {
                strncpy(temp_username, login_input_buffer, 31);
                login_set_username(temp_username);
                
                print_str("\n");
                print_str("Nome do computador: ");
                login_state = 1;
            }
        } else if (login_state == 1) {
            // Esperava hostname
            if (login_input_index > 0) {
                strncpy(temp_hostname, login_input_buffer, 31);
                login_set_hostname(temp_hostname);
                login_state = 2;
                print_str("\n\n");
                
                // Inicializar o shell após login
                if (!shell_initialized) {
                    terminal_start();
                    shell_initialized = 1;
                }
            } else {
                // Usar padrão se vazio
                login_set_hostname("imagineos");
                login_state = 2;
                print_str("\n\n");
                
                // Inicializar o shell após login
                if (!shell_initialized) {
                    terminal_start();
                    shell_initialized = 1;
                }
            }
        }
        
        login_input_index = 0;
        memset(login_input_buffer, 0, 256);
        
        return;
    }
    
    if (c == '\b') {
        // Backspace
        if (login_input_index > 0) {
            login_input_index--;
            login_input_buffer[login_input_index] = '\0';
            // Usar a função backspace de print.c que gerencia o cursor
            backspace();
        }
        return;
    }
    
    // Caractere normal
    if (login_input_index < 255 && c >= 32 && c < 127) {
        login_input_buffer[login_input_index] = c;
        login_input_index++;
        login_input_buffer[login_input_index] = '\0';
        print_char(c);
    }
}

void login_prompt(void) {
    print_clear();
    print_str("\n");
    print_str("  Welcome to ImagineOS!\n");
    print_str("  MELISSA R1 - Sep 2026 Release\n");
    print_str("\n");
    
    print_str("Usuario: ");
    login_state = 0;
    login_input_index = 0;
}

// Função para verificar se o login foi concluído
int login_is_complete(void) {
    return login_state == 2;
}
