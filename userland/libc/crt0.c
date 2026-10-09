#include <stdlib.h>
#include <unistd.h>
#include "dreamcore.h"

extern int main(int argc, char **argv);

__attribute__((noreturn))
void c_runtime_init(int argc, char **argv, int envc, char **envp)
{
    (void)envc;
    if (dc_syscall0(DC_SYS_ABI_VERSION) != 3) {
        _exit(2);
    }
    environ = envp;
    _exit(main(argc, argv));
}
