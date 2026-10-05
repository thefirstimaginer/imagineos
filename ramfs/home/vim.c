#include "dreamcore.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_CAPACITY 4096
#define PATH_CAPACITY 256
#define COMMAND_CAPACITY 8

static char document[BUFFER_CAPACITY];
static unsigned long document_length;
static unsigned long cursor_position;
static char filename[PATH_CAPACITY];

static void write_text(const char *text)
{
    write(STDOUT_FILENO, text, strlen(text));
}

static long read_character_input(void)
{
    unsigned char character;
    return read(STDIN_FILENO, &character, 1) == 1 ? character : -1;
}

static void show_status(const char *mode)
{
    write_text("\n\n");
    write_text(filename);
    write_text(" | ");
    write_text(mode);
    write_text(" | h/l: mover  i/a: inserir  x: apagar  :wq: salvar e sair\n");
}

static void redraw(const char *mode)
{
    dc_clear();
    printf("ImagineOS Vim - %s\n----------------------------\n", filename);
    for (unsigned long index = 0; index < document_length; index++) {
        if (index == cursor_position) write_text("|");
        dc_write(document + index, 1);
    }
    if (cursor_position == document_length) write_text("|");
    show_status(mode);
}

static int save_document(void)
{
    int descriptor = open(filename, O_WRONLY | O_TRUNC);
    if (descriptor < 0 && errno == 2) {
        descriptor = open(filename, O_WRONLY | O_CREAT, 0666);
    }
    if (descriptor < 0) {
        write_text("\nErro ao salvar (arquivo limitado a 4096 bytes).\n");
        return 0;
    }
    unsigned long written = 0;
    while (written < document_length) {
        long result = write(descriptor, document + written, document_length - written);
        if (result <= 0) {
            close(descriptor);
            write_text("\nErro ao salvar (arquivo limitado a 4096 bytes).\n");
            return 0;
        }
        written += (unsigned long)result;
    }
    close(descriptor);
    write_text("\nArquivo salvo.\n");
    return 1;
}

static void load_document(void)
{
    int descriptor = open(filename, O_RDONLY);
    if (descriptor < 0 && errno == 2) {
        descriptor = open(filename, O_CREAT | O_RDWR, 0666);
    }
    if (descriptor < 0) return;
    while (document_length < BUFFER_CAPACITY) {
        long result = read(descriptor, document + document_length,
                           BUFFER_CAPACITY - document_length);
        if (result <= 0) break;
        document_length += (unsigned long)result;
    }
    close(descriptor);
}

static int read_command(char *command)
{
    unsigned long length = 0;
    write_text(":");
    for (;;) {
        long input = read_character_input();
        if (input == '\r' || input == '\n') break;
        if (input == 8 || input == 127) {
            if (length) length--;
            continue;
        }
        if (input < 32 || input > 126 || length + 1 >= COMMAND_CAPACITY) continue;
        command[length++] = (char)input;
    }
    command[length] = 0;
    return length != 0;
}

static void append_character(long input)
{
    if (document_length >= BUFFER_CAPACITY) return;
    if (input == '\r') input = '\n';
    if (input == 8 || input == 127) {
        if (cursor_position) {
            memmove(document + cursor_position - 1, document + cursor_position,
                    document_length - cursor_position);
            cursor_position--;
            document_length--;
        }
        return;
    }
    if (input == '\n' || (input >= 32 && input <= 126)) {
        memmove(document + cursor_position + 1, document + cursor_position,
                document_length - cursor_position);
        document[cursor_position++] = (char)input;
        document_length++;
    }
}

int main(int argc, char **argv)
{
    const char *default_path = "/home/untitled.c";
    const char *requested_path = argc > 1 ? argv[1] : default_path;
    unsigned long path_length = dc_strlen(requested_path);
    if (path_length >= PATH_CAPACITY) {
        write_text("Caminho longo demais.\n");
        return 1;
    }
    for (unsigned long index = 0; index <= path_length; index++) filename[index] = requested_path[index];

    load_document();
    redraw("NORMAL");

    int inserting = 0;
    for (;;) {
        long input = read_character_input();
        if (inserting) {
            if (input == 27) {
                inserting = 0;
                redraw("NORMAL");
            } else {
                append_character(input);
                redraw("INSERT");
            }
            continue;
        }

        if (input == 'i') {
            inserting = 1;
            redraw("INSERT");
        } else if (input == 'a') {
            if (cursor_position < document_length) cursor_position++;
            inserting = 1;
            redraw("INSERT");
        } else if (input == 'h') {
            if (cursor_position) cursor_position--;
            redraw("NORMAL");
        } else if (input == 'l') {
            if (cursor_position < document_length) cursor_position++;
            redraw("NORMAL");
        } else if (input == 'x') {
            if (cursor_position < document_length) {
                memmove(document + cursor_position, document + cursor_position + 1,
                        document_length - cursor_position - 1);
                document_length--;
            }
            redraw("NORMAL");
        } else if (input == ':') {
            char command[COMMAND_CAPACITY];
            if (!read_command(command)) continue;
            if (command[0] == 'w' && command[1] == 'q' && command[2] == 0) {
                if (save_document()) return 0;
            } else if (command[0] == 'w' && command[1] == 0) {
                save_document();
                redraw("NORMAL");
            } else if (command[0] == 'q' && command[1] == 0) {
                return 0;
            } else {
                write_text("\nComando desconhecido. Use :w, :q ou :wq.\n");
            }
        }
    }
}
