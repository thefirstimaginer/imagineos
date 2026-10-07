#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void print_usage(void)
{
    const char *message = "usage: tcc source.c [-o output]\n";
    write(2, message, strlen(message));
}

int main(int argc, char **argv)
{
    (void)argv;
    if (argc < 2) {
        print_usage();
        return 1;
    }

    fputs("tcc: native TinyCC backend is not ported to Dreamcore yet\n", stderr);
    return 1;
}