/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_BIGSTRING_H_INCLUDED
#define CMC_EML_BIGSTRING_H_INCLUDED

#include "io.h"

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct bigstring_t
{
    char*   buf;
    size_t  size;
    ssize_t cur;
    size_t  max;
}* bigstring_p;

extern int bigstring_init(bigstring_p);

/**
 * Append the NUL-terminated string `str` to B.
 */
extern int bigstring_append(bigstring_p B, const char* str);
extern int bigstring_append_file(bigstring_p B, file_p F);

/**
 * Append to B a list of NUL-terminated strings.
 * The variadic list must be NULL-terminated.
 */
extern int bigstring_appendv(bigstring_p B, ...);

extern int bigstring_free(bigstring_p);

#endif /* CMC_EML_BIGSTRING_H_INCLUDED */
