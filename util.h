/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_UTIL_H_INCLUDED
#define CMC_EML_UTIL_H_INCLUDED

#include <stddef.h>
#include <sys/types.h>

#define FS_BUFFER_SIZE 65536

typedef struct rbuffer_t
{
    int     fd;
    char    buffer[FS_BUFFER_SIZE];
    ssize_t count;
    ssize_t cur;
}* rbuffer_p;

typedef struct wbuffer_t
{
    int    fd;
    char   buffer[FS_BUFFER_SIZE];
    size_t cur;
}* wbuffer_p;

extern void    rbuffer_init(rbuffer_p B, int fd);
extern ssize_t rbuffer_read(rbuffer_p B, char* dst, ssize_t sz);

extern void wbuffer_init(wbuffer_p B, int fd);
extern void wbuffer_put(wbuffer_p B, char* buf, int sz);
extern void wbuffer_flush(wbuffer_p B);

extern void writee(int fd, char* buf, size_t);
extern void writeev(int fd, ...);

#endif /* CMC_EML_UTIL_H_INCLUDED */
