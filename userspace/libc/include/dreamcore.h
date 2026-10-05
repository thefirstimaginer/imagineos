#ifndef DREAMCORE_H
#define DREAMCORE_H

#define DC_SYS_WRITE 1
#define DC_SYS_READ 2
#define DC_SYS_EXIT 4
#define DC_SYS_CLEAR 6
#define DC_SYS_READ_FILE 10
#define DC_SYS_WRITE_FILE 15
#define DC_SYS_OPEN 16
#define DC_SYS_READ_FD 17
#define DC_SYS_WRITE_FD 18
#define DC_SYS_CLOSE 19
#define DC_SYS_LSEEK 20
#define DC_SYS_STAT 21
#define DC_SYS_FSTAT 22

static inline long dc_syscall0(long number)
{
    long result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(number) : "memory", "rcx", "r11");
    return result;
}

static inline long dc_syscall1(long number, long first)
{
    long result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(number), "D"(first) : "memory", "rcx", "r11");
    return result;
}

static inline long dc_syscall2(long number, long first, long second)
{
    long result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(number), "D"(first), "S"(second) : "memory", "rcx", "r11");
    return result;
}

static inline long dc_syscall3(long number, long first, long second, long third)
{
    long result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(number), "D"(first), "S"(second), "d"(third) : "memory", "rcx", "r11");
    return result;
}

static inline long dc_syscall4(long number, long first, long second, long third, long fourth)
{
    long result;
    register long fourth_register __asm__("r10") = fourth;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(number), "D"(first), "S"(second), "d"(third), "r"(fourth_register) : "memory", "rcx", "r11");
    return result;
}

static inline long dc_write(const void *bytes, unsigned long length)
{
    const unsigned char *cursor = bytes;
    unsigned long written = 0;
    while (written < length) {
        unsigned long chunk = length - written;
        if (chunk > 512) chunk = 512;
        long result = dc_syscall3(DC_SYS_WRITE, (long)(cursor + written), (long)chunk, 0);
        if (result <= 0) return result < 0 ? result : (long)written;
        written += (unsigned long)result;
    }
    return (long)written;
}

static inline long dc_read_character(void)
{
    return dc_syscall0(DC_SYS_READ);
}

static inline long dc_open(const char *path, unsigned long length, int flags)
{
    return dc_syscall3(DC_SYS_OPEN, (long)path, (long)length, flags);
}

static inline long dc_read_fd(int descriptor, void *buffer, unsigned long count)
{
    return dc_syscall3(DC_SYS_READ_FD, descriptor, (long)buffer, (long)count);
}

static inline long dc_write_fd(int descriptor, const void *buffer, unsigned long count)
{
    return dc_syscall3(DC_SYS_WRITE_FD, descriptor, (long)buffer, (long)count);
}

static inline long dc_close(int descriptor)
{
    return dc_syscall1(DC_SYS_CLOSE, descriptor);
}

static inline long dc_lseek(int descriptor, long offset, int whence)
{
    return dc_syscall3(DC_SYS_LSEEK, descriptor, offset, whence);
}

static inline long dc_clear(void)
{
    return dc_syscall0(DC_SYS_CLEAR);
}

static inline unsigned long dc_strlen(const char *text)
{
    unsigned long length = 0;
    while (text[length]) length++;
    return length;
}

static inline long dc_read_file(const char *path, char *buffer, unsigned long capacity)
{
    return dc_syscall4(DC_SYS_READ_FILE, (long)path, (long)dc_strlen(path), (long)buffer, (long)capacity);
}

static inline long dc_write_file(const char *path, const char *buffer, unsigned long length)
{
    return dc_syscall4(DC_SYS_WRITE_FILE, (long)path, (long)dc_strlen(path), (long)buffer, (long)length);
}

static inline void dc_exit(int status)
{
    (void)status;
    dc_syscall0(DC_SYS_EXIT);
    for (;;) __asm__ volatile("pause");
}

#endif