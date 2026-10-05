#include <unistd.h>

#include "dreamcore.h"

int errno;

static ssize_t set_error(int error)
{
    errno = error;
    return -1;
}

ssize_t write(int descriptor, const void *buffer, size_t count)
{
    if (descriptor != STDOUT_FILENO && descriptor != STDERR_FILENO) {
        return set_error(9);
    }
    long result = dc_write(buffer, count);
    if (result < 0) {
        return set_error((int)-result);
    }
    return result;
}

ssize_t read(int descriptor, void *buffer, size_t count)
{
    if (descriptor != STDIN_FILENO) {
        return set_error(9);
    }
    if (count == 0) {
        return 0;
    }

    long input = dc_read_character();
    if (input < 0) {
        return set_error((int)-input);
    }
    unsigned long codepoint = (unsigned long)input;
    unsigned char encoded[4];
    size_t length;
    if (codepoint < 0x80) {
        encoded[0] = (unsigned char)codepoint;
        length = 1;
    } else if (codepoint < 0x800) {
        encoded[0] = 0xc0 | (unsigned char)(codepoint >> 6);
        encoded[1] = 0x80 | (unsigned char)(codepoint & 0x3f);
        length = 2;
    } else if (codepoint < 0x10000) {
        encoded[0] = 0xe0 | (unsigned char)(codepoint >> 12);
        encoded[1] = 0x80 | (unsigned char)((codepoint >> 6) & 0x3f);
        encoded[2] = 0x80 | (unsigned char)(codepoint & 0x3f);
        length = 3;
    } else if (codepoint <= 0x10ffff) {
        encoded[0] = 0xf0 | (unsigned char)(codepoint >> 18);
        encoded[1] = 0x80 | (unsigned char)((codepoint >> 12) & 0x3f);
        encoded[2] = 0x80 | (unsigned char)((codepoint >> 6) & 0x3f);
        encoded[3] = 0x80 | (unsigned char)(codepoint & 0x3f);
        length = 4;
    } else {
        return set_error(84);
    }
    if (count < length) {
        return set_error(22);
    }
    unsigned char *output = buffer;
    for (size_t index = 0; index < length; index++) {
        output[index] = encoded[index];
    }
    return (ssize_t)length;
}

void _exit(int status)
{
    dc_exit(status);
}