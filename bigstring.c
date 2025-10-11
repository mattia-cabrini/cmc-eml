/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "bigstring.h"
#include "error.h"
#include <errno.h>
#include <stdarg.h>

#define BIGSTRING_START_SIZE 65536

static int bigstring_realloc(bigstring_p);

static int bigstring_realloc(bigstring_p S)
{
    S->buf = realloc(S->buf, S->size * 2);

    if (errno)
        return errno + ERRNO_SPLIT;

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

    if (errno)
        return errno + ERRNO_SPLIT;

    return OK;
}

int bigstring_append_file(bigstring_p S, FILE* fp)
{
    int    ret        = OK;
    size_t read_bytes = 0;
    size_t buf_fill   = 0;

    assert(S->cur >= 0, FATAL_SIGSEGV, "bigstring_append_file: !init (cur)");
    assert(S->buf != 0, FATAL_SIGSEGV, "bigstring_append_file: !init (buf)");

    while (ret == OK && !feof(fp))
    {
        buf_fill   = S->max - (size_t)S->cur;
        read_bytes = fread(S->buf + S->cur, sizeof(S->buf[0]), buf_fill, fp);
        S->cur += (ssize_t)read_bytes;

        /* Buffer filled, realloc */
        if ((size_t)S->cur == S->max)
            ret = bigstring_realloc(S);
    }

    return ret;
}

int bigstring_append(bigstring_p S, char* str)
{
    int ret = OK;

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

    return OK;
}

/* NULL-value terminated */
int bigstring_appendv(bigstring_p S, ...)
{
    va_list args;
    char*   str;
    int     ret = OK;

    va_start(args, S);

    while (!ret && (str = va_arg(args, char*)) != NULL)
    {
        ret = bigstring_append(S, str);
    }

    va_end(args);
    return ret;
}

int bigstring_free(bigstring_p S)
{
    free(S->buf);
    S->cur  = -1;
    S->max  = 0;
    S->size = 0;

    if (errno)
        return errno + ERRNO_SPLIT;

    return OK;
}
