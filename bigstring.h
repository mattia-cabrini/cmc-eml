/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_BIGSTRING_H_INCLUDED
#define CMC_EML_BIGSTRING_H_INCLUDED

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

extern int bigstring_append(bigstring_p, char*);
extern int bigstring_append_file(bigstring_p, int fd);

/* NULL-value terminated */
extern int bigstring_appendv(bigstring_p, ...);

extern int bigstring_free(bigstring_p);

#endif /* CMC_EML_BIGSTRING_H_INCLUDED */
