#include "input.h"
#include "idt.h"
#include "ps2.h"
#include <stdint.h>

#define INPUT_BUFFER_SIZE 256

static volatile char input_buffer[INPUT_BUFFER_SIZE];
static volatile unsigned int input_read_position;
static volatile unsigned int input_write_position;
static volatile int input_shift;
static volatile int input_caps_lock;

static char scan_code_to_ascii(uint8_t scan_code) {
    static const char key_map[0x40] = {
        [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
        [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
        [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
        [0x0E] = '\b', [0x0F] = '\t',
        [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
        [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
        [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
        [0x1C] = '\n', [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd',
        [0x21] = 'f', [0x22] = 'g', [0x23] = 'h', [0x24] = 'j',
        [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'',
        [0x29] = '`', [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x',
        [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n',
        [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
        [0x39] = ' '
    };

    if (scan_code >= sizeof(key_map)) return 0;
    return key_map[scan_code];
}

static void input_keyboard_handler(void) {
    uint8_t scan_code = ps2_read_scan_code();
    char character;

    if (scan_code == 0x2A || scan_code == 0x36) {
        input_shift = 1;
        return;
    }
    if (scan_code == 0xAA || scan_code == 0xB6) {
        input_shift = 0;
        return;
    }
    if (scan_code == 0x3A) {
        input_caps_lock = !input_caps_lock;
        return;
    }
    if (scan_code & 0x80) return;

    character = scan_code_to_ascii(scan_code);
    if (character == 0) return;
    if ((input_shift ^ input_caps_lock) && character >= 'a' && character <= 'z') {
        character -= 'a' - 'A';
    }

    if (input_shift) {
        switch (character) {
            case '1': character = '!'; break;
            case '2': character = '@'; break;
            case '3': character = '#'; break;
            case '4': character = '$'; break;
            case '5': character = '%'; break;
            case '6': character = '^'; break;
            case '7': character = '&'; break;
            case '8': character = '*'; break;
            case '9': character = '('; break;
            case '0': character = ')'; break;
            case '-': character = '_'; break;
            case '=': character = '+'; break;
            case '[': character = '{'; break;
            case ']': character = '}'; break;
            case ';': character = ':'; break;
            case '\'': character = '"'; break;
            case '`': character = '~'; break;
            case '\\': character = '|'; break;
            case ',': character = '<'; break;
            case '.': character = '>'; break;
            case '/': character = '?'; break;
        }
    }

    if (input_write_position - input_read_position < INPUT_BUFFER_SIZE) {
        input_buffer[input_write_position % INPUT_BUFFER_SIZE] = character;
        input_write_position++;
    }
}

void input_init(void) {
    input_read_position = 0;
    input_write_position = 0;
    input_shift = 0;
    input_caps_lock = 0;
    idt_set_handler_keyboard(input_keyboard_handler);
}

long input_read(char *buffer, unsigned long count) {
    unsigned long copied = 0;

    while (copied < count) {
        while (input_read_position == input_write_position) {
            __asm__ volatile("sti\n hlt" ::: "memory");
        }
        buffer[copied++] = input_buffer[input_read_position % INPUT_BUFFER_SIZE];
        input_read_position++;
        if (buffer[copied - 1] == '\n') break;
    }
    return (long)copied;
}
