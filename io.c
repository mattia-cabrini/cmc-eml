/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "feat.h"

#include "error.h"
#include "io.h"
#include "util.h"

#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

void file_set_null(file_p F)
{
    if (F == NULL)
        return;

    F->fd = -1;
}

int file_is_init(file_p F)
{
    if (F == NULL)
        return 0;

    return F->fd >= 0;
}

int file_open(file_p F, const char* path, int flags, mode_t mode)
{
    assert(F != NULL, FATAL_LOGIC, "file_open: invalid file");

    F->fd = open(path, flags, mode);

    if (F->fd < 0)
        return errno + ERRNO_SPLIT;

#ifdef DEBUG
    fprintf(
        stderr,
        "DEBUG open fd %d with flags %d and mode %d\n",
        F->fd,
        flags,
        (int)mode
    );
#endif

    return OK;
}

int file_open_tmp(file_p F)
{
    char template[] = "/tmp/XXXXXX";
    int res;

    assert(F != NULL, FATAL_LOGIC, "file_open_tmp: invalid file");

    F->fd = mkstemp(template);
    if (F->fd < 0)
        return errno + ERRNO_SPLIT;

#ifdef DEBUG
    fprintf(
        stderr, "DEBUG open tmp file with fd %d; path: %s\n", F->fd, template
    );
    fprintf(stderr, "DEBUG unlink path %s\n", template);
#endif

    res = unlink(template);
    assert(res != -1, errno + ERRNO_SPLIT, "file_open_tmp: unlink");

    return OK;
}

void file_set_fd(file_p F, int fd)
{
    assert(F != NULL, FATAL_LOGIC, "file_set_fd: invalid file");

    F->fd = fd;
}

ssize_t file_read(file_p F, char* buf, size_t max)
{
    assert(F != NULL, FATAL_LOGIC, "file_read: invalid file");
    F->last_rb = read(F->fd, buf, max);

#ifdef DEBUG
    fprintf(stderr, "DEBUG read %d bytes from fd %d\n", (int)F->last_rb, F->fd);
#endif

    return F->last_rb;
}

int file_write(file_p F, const char* buf, size_t count)
{
    size_t  written = 0;
    ssize_t res;
    int     errcount = 0;

    assert(F != NULL, FATAL_LOGIC, "file_write: invalid file");

    while (written < count)
    {
        res = write(F->fd, buf + written, count - written);
        if (res > 0)
        {
            written = written + (size_t)res;
            continue;
        }

        if (res == 0)
            return EIO + ERRNO_SPLIT;

        switch (errno)
        {
        case EINTR:
            if (errcount < 10)
            {
                ++errcount;
                continue;
            }
            return errno + ERRNO_SPLIT;

        case EAGAIN:
            /* case EWOULDBLOCK: */
            continue;

        default:
            return errno + ERRNO_SPLIT;
        }
    }

#ifdef DEBUG
    fprintf(
        stderr,
        "DEBUG written %d bytes from fd %d; requested %d\n",
        (int)written,
        F->fd,
        (int)count
    );
#endif

    return OK;
}

int file_write_str(file_p F, const char* str)
{
    assert(F != NULL, FATAL_LOGIC, "file_write_str: invalid file");

    return file_write(F, str, strlen(str) * sizeof(char));
}

int file_write_strv(file_p F, ...)
{
    va_list     args;
    const char* str;
    int         res = OK;

    assert(F != NULL, FATAL_LOGIC, "file_write_strv: invalid file");

    va_start(args, F);

    while ((str = va_arg(args, const char*)) != NULL)
    {
        res = file_write_str(F, str);
        if (res != OK)
            break;
    }

    va_end(args);
    return res;
}

int file_copy(file_p dst, file_p src)
{
    int     res;
    ssize_t sz;
    char    buf[8192];

    assert(dst != NULL, FATAL_LOGIC, "file_copy: invalid file (dst)");
    assert(src != NULL, FATAL_LOGIC, "file_copy: invalid file (src)");

    if (file_isreg(src))
    {
        res = file_seek(src, 0, SEEK_SET);
        if (res != OK)
            return res;
    }

    while ((sz = file_read(src, buf, sizeof(buf))) > 0)
    {
        res = file_write(dst, buf, (size_t)sz);

        if (res != OK)
            return res;
    }

    if (sz < 0)
        return errno + ERRNO_SPLIT;

    return OK;
}

int file_seek(file_p F, off_t off, int whence)
{
    off_t out;

    assert(F != NULL, FATAL_LOGIC, "file_seek: invalid file");

#ifdef DEBUG
    fprintf(
        stderr,
        "DEBUG seek on fd %d; off: %d; whence: %d\n",
        F->fd,
        (int)off,
        (int)whence
    );
#endif

    out = lseek(F->fd, off, whence);
    if (out == -1)
        return errno + ERRNO_SPLIT;

    return OK;
}

off_t file_cur(file_p F)
{
    assert(F != NULL, FATAL_LOGIC, "file_cur: invalid file");

#ifdef DEBUG
    fprintf(
        stderr,
        "DEBUG file_cur: seek on fd %d; off: 0; whence: %d\n",
        F->fd,
        (int)SEEK_CUR
    );
#endif

    return lseek(F->fd, 0, SEEK_CUR);
}

int file_isreg(file_p F)
{
    struct stat s;
    int         res;

    assert(F != NULL, FATAL_LOGIC, "file_isreg: invalid file");

    res = fstat(F->fd, &s);
    assert(res == 0, errno + ERRNO_SPLIT, "file_isreg: fstat: could not stat");

    return S_ISREG(s.st_mode);
}

void file_close(file_p F)
{
    assert(F != NULL, FATAL_LOGIC, "file_close: invalid file");

    close(F->fd);
    F->fd = -1;
}

ssize_t file_last_rb(file_p F) { return F->last_rb; }
