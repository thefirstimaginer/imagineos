#pragma once

void terminal_init(void);
void terminal_start(void);
void terminal_input(char c);
void terminal_history_up(void);
void terminal_history_down(void);
void shell_add_char(char c);
void shell_print_prompt(void);