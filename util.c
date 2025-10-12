/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "util.h"
#include "error.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

static void rbuffer_next(rbuffer_p B);

void rbuffer_init(rbuffer_p B, int fd)
{
    B->fd    = fd;
    B->count = 0;
    B->cur   = 0;
}

static void rbuffer_next(rbuffer_p B)
{
    B->count = read(B->fd, B->buffer, sizeof(B->buffer));
    B->cur   = 0;
    assert(B->count >= 0, FATAL_SIGSEGV, "rbuffer_next: could not read");
}

ssize_t rbuffer_read(rbuffer_p B, char* dst, ssize_t sz)
{
    int dst_i = 0;

    for (dst_i = 0; dst_i < sz;)
    {
        if (B->cur == B->count)
        {
            rbuffer_next(B);
            if (B->count == 0)
                break;
        }

        for (; B->cur < B->count && dst_i < sz; ++dst_i)
        {
            dst[dst_i] = B->buffer[B->cur];
            ++B->cur;
        }
    }

    return dst_i;
}

void wbuffer_init(wbuffer_p B, int fd)
{
    B->fd  = fd;
    B->cur = 0;
}

void wbuffer_put(wbuffer_p B, char* buf, int sz)
{
    int cur;

    for (cur = 0; cur < sz;)
    {
        if (B->cur == sizeof(B->buffer))
            wbuffer_flush(B);

        for (; cur < sz && B->cur < sizeof(B->buffer); ++cur)
        {
            B->buffer[B->cur] = buf[cur];
            ++B->cur;
        }
    }
}

void wbuffer_flush(wbuffer_p B)
{
    ssize_t res;

    res = write(B->fd, B->buffer, B->cur);
    assert(
        (size_t)res == B->cur, FATAL_SIGSEGV, "wbuffer_flush: could not write"
    );
    B->cur = 0;
}

void writee(int fd, char* buf, size_t sz)
{
    ssize_t res;
    res = write(fd, buf, sz);

    if ((size_t)res != sz)
    {
        sprintf(error_message, "write on %d failed", fd);
        assert((size_t)res == sz, FATAL_SIGSEGV, error_message);
    }
}

void writeev(int fd, ...)
{
    va_list args;
    char*   str;

    va_start(args, fd);

    while ((str = va_arg(args, char*)) != NULL)
        writee(fd, str, strlen(str) * sizeof(char));

    va_end(args);
}
