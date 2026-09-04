#pragma once

void shell_init(void);
void shell_run(char* args);
void shell_execute_line(const char* line);
const char* shell_history_up(void);
const char* shell_history_down(void);
