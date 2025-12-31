#pragma once

void shell_init();
void shell_handle_enter(void);                             // process command entered at prompt)
void shell_print_prompt();                                 // print shell prompt and set editable area
int string_to_int(char* str, int* next_pos);