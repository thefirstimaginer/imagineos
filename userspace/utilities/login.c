/* Imagine Operating System - vR1_0.1.0 - Codename "Jessica"
 * Copyright (c) 2026, Adryan Alcantara & TeamImagine
 * Licensed Under IGPLv1
 *
 * Login Module for ImagineOS.
 */

#include "print.h"
#include "stdimagine.h"

// Variáveis globais de usuário e hostname
char current_username[32] = "root";
char current_hostname[32] = "imagineos";

void login_set_username(const char* username) {
    strncpy(current_username, username, 31);
}

void login_set_hostname(const char* hostname) {
    strncpy(current_hostname, hostname, 31);
}

const char* login_get_username(void) {
    return current_username;
}

const char* login_get_hostname(void) {
    return current_hostname;
}

void login_init(void) {
    // Inicialização - pode ser chamada durante o boot
}

void login_run(void) {
    // Simples re-login durante a sessão
    print_str("Usuário: ");
    // Aqui entraria a lógica de leitura de entrada
    // Por enquanto, apenas mostra mensagem
    print_str("Login não implementado interativamente ainda.\n");
}
