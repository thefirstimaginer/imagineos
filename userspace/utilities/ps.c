#include "print.h"
#include "process.h"

void ps_init(void) {
}

void ps_run(char* args) {
    (void)args;
    Process* process = process_list;

    print_str("PID  STATE      PROGRAM\n");
    while (process) {
        print_uint64_hex(process->pid);
        print_str("  ");
        print_str((char*)process_state_name(process->state));
        print_str("  ");
        print_str((char*)process->name);
        print_str("\n");
        process = process->next;
    }
}