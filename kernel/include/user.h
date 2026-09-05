#ifndef IMAGINEOS_USER_H
#define IMAGINEOS_USER_H

#include <stdint.h>

int user_init_from_multiboot(uint64_t multiboot_info);
int user_exec_service(const char *service_name);
void user_enter(uint64_t entry, uint64_t stack);

#endif