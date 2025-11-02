/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_UTIL_H_INCLUDED
#define CMC_EML_UTIL_H_INCLUDED

#include "feat.h"

#include "io.h"

#include <stddef.h>
#include <sys/types.h>

#define FS_BUFFER_SIZE 65536

typedef struct rbuffer_t
{
    file_p  F;
    char    buffer[FS_BUFFER_SIZE];
    ssize_t count;
    ssize_t cur;
}* rbuffer_p;

typedef struct wbuffer_t
{
    file_p F;
    char   buffer[FS_BUFFER_SIZE];
    size_t cur;
}* wbuffer_p;

extern void    rbuffer_init(rbuffer_p B, file_p F);
extern ssize_t rbuffer_read(rbuffer_p B, char* dst, ssize_t sz);

extern void wbuffer_init(wbuffer_p B, file_p F);
extern void wbuffer_put(wbuffer_p B, char* buf, int sz);
extern void wbuffer_flush(wbuffer_p B);

extern int  strnappend(char* dst, const char* src, int n);
extern void strnappendv(char* dst, int n, ...);

#endif /* CMC_EML_UTIL_H_INCLUDED */
