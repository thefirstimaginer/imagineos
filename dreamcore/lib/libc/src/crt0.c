#include <unistd.h>

extern int main(int argc, char **argv);

void _start(void) {
    exit(main(0, (char **)0));
}