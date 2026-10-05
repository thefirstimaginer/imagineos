#include "dreamcore.h"

extern int main(int argc, char **argv);

void _start(int argc, char **argv)
{
    dc_exit(main(argc, argv));
}