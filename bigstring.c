/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "bigstring.h"
#include "error.h"
#include "io.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define BIGSTRING_START_SIZE 65536

static int bigstring_realloc(bigstring_p);

static int bigstring_realloc(bigstring_p S)
{
    char* tmp;

    if (S->size > SSIZE_MAX / 2)
    {
        return EOVERFLOW + ERRNO_SPLIT;
    }

    tmp = realloc(S->buf, S->size * 2);

    if (tmp == NULL)
    {
        free(S->buf);
        S->buf  = NULL;
        S->cur  = -1;
        S->size = 0;
        S->max  = 0;

        return errno + ERRNO_SPLIT;
    }

    S->buf = tmp;

    S->size *= 2;
    S->max = S->size / sizeof(char);

    return OK;
}

int bigstring_init(bigstring_p S)
{
    S->cur  = 0;
    S->size = BIGSTRING_START_SIZE;
    S->max  = S->size / sizeof(char);
    S->buf  = (char*)malloc(BIGSTRING_START_SIZE);

    if (S->buf == NULL)
        return errno + ERRNO_SPLIT;

    return OK;
}

int bigstring_append_file(bigstring_p S, file_p F)
{
    int     ret        = OK;
    ssize_t read_bytes = 1; /* To enter while loop at least once */
    size_t  buf_fill   = 0;

    assert(S->cur >= 0, FATAL_SIGSEGV, "bigstring_append_file: !init (cur)");
    assert(S->buf != 0, FATAL_SIGSEGV, "bigstring_append_file: !init (buf)");

    while (ret == OK && read_bytes > 0)
    {
        buf_fill = S->max - (size_t)S->cur;
        read_bytes =
            file_read(F, S->buf + S->cur, sizeof(S->buf[0]) * buf_fill);
        assert(
            read_bytes >= 0,
            FATAL_SIGSEGV,
            "bigstring_append_file: could not read file"
        );
        S->cur += read_bytes;

        /* Buffer filled, realloc */
        if ((size_t)S->cur == S->max)
            ret = bigstring_realloc(S);
    }

    return ret;
}

int bigstring_append(bigstring_p S, const char* str)
{
    int ret = OK;

    assert(str != NULL, FATAL_LOGIC, "bigstring_append: str not valid");

    while (*str && ret == OK)
    {
        while ((size_t)S->cur < S->max && (S->buf[S->cur] = *str))
        {
            ++str;
            ++S->cur;
        }

        /* Could not copy the whole string -> extend buffer */
        if (*str)
            ret = bigstring_realloc(S);
    }

    return ret;
}

/* NULL-value terminated */
int bigstring_appendv(bigstring_p S, ...)
{
    va_list     args;
    const char* str;
    int         ret = OK;

    va_start(args, S);

    while (ret == OK && (str = va_arg(args, const char*)) != NULL)
    {
        ret = bigstring_append(S, str);
    }

    va_end(args);
    return ret;
}

int bigstring_free(bigstring_p S)
{
    free(S->buf);
    S->buf  = NULL;
    S->cur  = -1;
    S->max  = 0;
    S->size = 0;

    return OK;
}
