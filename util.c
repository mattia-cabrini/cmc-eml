/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "feat.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "util.h"

static void rbuffer_next(rbuffer_p B);

void rbuffer_init(rbuffer_p B, file_p F)
{
    B->F     = F;
    B->count = 0;
    B->cur   = 0;
}

static void rbuffer_next(rbuffer_p B)
{
    B->count = file_read(B->F, B->buffer, sizeof(B->buffer));
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

void wbuffer_init(wbuffer_p B, file_p F)
{
    B->F   = F;
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

    res = file_write(B->F, B->buffer, B->cur);
    assert(res == OK, FATAL_SIGSEGV, "wbuffer_flush: could not write");
    B->cur = 0;
}

int strnappend(char* dst, const char* src, int n)
{
    int on = n;

    if (n <= 0)
        return -1;

    /* Copy until n > 1 in order to leave space for NUL */
    for (; n > 1 && *src; --n)
    {
        *dst = *src;

        ++dst;
        ++src;
    }

    /* At least one character has not been copied -> error */
    if (*src)
        return -1;

    *dst = '\0';

    return *src ? -1 : on - n;
}

int strnappendv(char* dst, int n, ...)
{
    int     res;
    va_list args;
    va_start(args, n);
    res = strnappendvv(dst, n, args);
    va_end(args);
    return res;
}

int strnappendvv(char* dst, int n, va_list args)
{
    /* Number of non-NUL characters copied so far.
     * If cpin is -1 cp is undefined.
     */
    int cp = 0;

    /* Number of non-NUL character copied by a single strnappend call. */
    int cpin        = 0;

    const char* str = NULL;

    while (cpin >= 0 && (str = va_arg(args, const char*)) != NULL)
    {
        cpin = strnappend(dst + cp, str, n - cp);
        cp += cpin;
    }

    return cpin < 0 ? cpin : cp;
}
