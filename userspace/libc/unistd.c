#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "dreamcore.h"

int errno;

static ssize_t set_error(int error)
{
    errno = error;
    return -1;
}

ssize_t write(int descriptor, const void *buffer, size_t count)
{
    long result = dc_write_fd(descriptor, buffer, count);
    if (result < 0) {
        return set_error((int)-result);
    }
    return result;
}

ssize_t read(int descriptor, void *buffer, size_t count)
{
    long result = dc_read_fd(descriptor, buffer, count);
    return result < 0 ? set_error((int)-result) : result;
}

int close(int descriptor)
{
    long result = dc_close(descriptor);
    return result < 0 ? (int)set_error((int)-result) : (int)result;
}

off_t lseek(int descriptor, off_t offset, int whence)
{
    long result = dc_lseek(descriptor, offset, whence);
    if (result < 0) {
        errno = (int)-result;
        return (off_t)-1;
    }
    return (off_t)result;
}

int open(const char *path, int flags, ...)
{
    long result = dc_open(path, dc_strlen(path), flags);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }
    return (int)result;
}

int stat(const char *path, struct stat *buffer)
{
    if (!buffer) {
        errno = 22;
        return -1;
    }
    long result = dc_syscall3(DC_SYS_STAT, (long)path, (long)dc_strlen(path), (long)buffer);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

int fstat(int descriptor, struct stat *buffer)
{
    if (!buffer) {
        errno = 22;
        return -1;
    }
    long result = dc_syscall2(DC_SYS_FSTAT, descriptor, (long)buffer);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

void _exit(int status)
{
    dc_exit(status);
}