#pragma once

#include "print.h"
#include "stdimagine.h"

extern char current_username[32];
extern char current_hostname[32];

void login_set_username(const char* username);
void login_set_hostname(const char* hostname);
const char* login_get_username(void);
const char* login_get_hostname(void);
void login_init(void);
void login_run(char* args);

// Função para fazer prompt de login interativo durante o boot
void login_prompt(void);
