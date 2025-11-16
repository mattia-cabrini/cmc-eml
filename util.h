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

/**
 * Copy at most n non-NUL character from src to `dst`.
 * An additional NUL is copied into `dst` in order to let `dst` be a valid
 * NUL-terminated string.
 *
 * RETURN
 * If it is not possible to copy all non-NUL characters from `src` to `dst` (n
 * too small), return -1.
 *
 * Otherwise, return the number of non-NUL characters copied into `dst`.
 *
 * WARNING
 * If -1 is returned, `dst` is set to empty string.
 */
extern int strnappend(char* dst, const char* src, int n);

/**
 * Forward `dst`, `n` and ... to strnappendvv and return its result.
 */
extern int strnappendv(char* dst, int n, ...);

/**
 * Copy all the strings in `args` into `dst`.
 * `n` is the maximum number of characters that `dst` can hold.
 *
 * `args` must be NULL-terminated.
 *
 * RETRUN
 * If it is not possible to copy all non-NUL characters from `args` to `dst` (n
 * too small), return -1.
 *
 * Otherwise, return the number of non-NUL character copied into `dst`.
 *
 * WARNING
 * If -1 is returned, `dst` is set to empty string.
 */
extern int strnappendvv(char* dst, int n, va_list args);

/**
 * Set all 'n' character in the buffer `buf` to an ASCII character between 'a'
 * and 'z'.
 *
 * The resulting buffer will not be NUL-terminated and therefore will not be a
 * valid C-string.
 */
void get_rand_string(char* buf, size_t n);

#endif /* CMC_EML_UTIL_H_INCLUDED */
