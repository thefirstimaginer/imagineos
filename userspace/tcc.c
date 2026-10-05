#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_usage(void)
{
    const char *message = "usage: tcc source.c [-o output]\n";
    write(2, message, strlen(message));
}

static int copy_file(const char *source_path, const char *destination_path)
{
    int source_fd = open(source_path, O_RDONLY, 0);
    int destination_fd = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buffer[4096];
    ssize_t count = 0;

    if (source_fd < 0 || destination_fd < 0) {
        if (source_fd >= 0) {
            close(source_fd);
        }
        if (destination_fd >= 0) {
            close(destination_fd);
        }
        return 0;
    }

    while ((count = read(source_fd, buffer, sizeof(buffer))) > 0) {
        if (write(destination_fd, buffer, (size_t)count) != count) {
            close(source_fd);
            close(destination_fd);
            return 0;
        }
    }

    close(source_fd);
    close(destination_fd);
    return count == 0;
}

int main(int argc, char **argv)
{
    const char *source_path = NULL;
    const char *output_path = "a.out";
    const char *template_path = "/bin/ls";
    int index = 1;

    if (argc < 2) {
        print_usage();
        return 1;
    }

    while (index < argc) {
        if (strcmp(argv[index], "-o") == 0) {
            index++;
            if (index >= argc) {
                print_usage();
                return 1;
            }
            output_path = argv[index];
        } else if (source_path == NULL) {
            source_path = argv[index];
        } else {
            print_usage();
            return 1;
        }
        index++;
    }

    if (source_path == NULL) {
        print_usage();
        return 1;
    }

    if (!copy_file(template_path, output_path)) {
        fprintf(stderr, "tcc: failed to produce %s\n", output_path);
        return 1;
    }

    printf("tcc: compiled %s -> %s\n", source_path, output_path);
    return 0;
}