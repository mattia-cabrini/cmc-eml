/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_ERROR_H_INCLUDED
#define CMC_EML_ERROR_H_INCLUDED

#include <errno.h>
#include <gpgme.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Not an error
 */
#define NOT_AN_ERROR -1

/* [0, 1000]
 * OK and FATAL(s)
 */
#define OK 0
#define FATAL_PARAM 2
#define FATAL_SIGSEGV 3
#define FATAL_LOGIC 4
#define FATAL_GPGME 5

/* (1e+3, 1e+6]
 * ¬FATAL ERROR(s)
 */
#define BUFFER_FULL 1001
#define STRING_TOO_LONG 1002
#define ILLEGAL_FORMAT 1003
#define NOT_FOUND 1004

/* For errors such as x > 1e+6, the actual error is errno = x - 10e+6 */
#define ERRNO_SPLIT 1000000

#define MAX_ERROR_SIZE 1024
extern char error_message[];

extern void error_setgpg(gpgme_error_t err);

#define return_iferr(ret)                                                      \
    {                                                                          \
        if ((ret))                                                             \
        {                                                                      \
            return (ret);                                                      \
        }                                                                      \
    }

#define return_void_iferr(ret)                                                 \
    {                                                                          \
        if ((ret))                                                             \
        {                                                                      \
            return;                                                            \
        }                                                                      \
    }

#define assert(cond, code, msg)                                                \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            if ((code) > 1000000)                                              \
                fprintf(                                                       \
                    stderr,                                                    \
                    "ERROR %d: %s\n---AT %s\n",                                \
                    (code) - 1000000,                                          \
                    strerror((code) - 1000000),                                \
                    msg                                                        \
                );                                                             \
            else                                                               \
                fprintf(stderr, "ERROR %d: %s\n", (code), msg);                \
                                                                               \
            exit(code);                                                        \
        }                                                                      \
    }

#define STRCPY_OR_TOOLONG(dst, src, dst_size, err_str)                         \
    {                                                                          \
        {                                                                      \
            size_t strcpy_or_toolong_tmp_size = strlen((src)) + 1;             \
            strcpy_or_toolong_tmp_size *= sizeof(char);                        \
            if (strcpy_or_toolong_tmp_size > (dst_size))                       \
            {                                                                  \
                strcpy(error_message, (err_str));                              \
                return STRING_TOO_LONG;                                        \
            }                                                                  \
            memcpy((dst), (src), strcpy_or_toolong_tmp_size);                  \
        }                                                                      \
    }

#define arr_assign_cont(V, index, N, val, STAT_VAR)                            \
    {                                                                          \
        if ((int)(index) >= (int)(N))                                          \
        {                                                                      \
            STAT_VAR = 1002;                                                   \
            continue;                                                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            (V)[(index)] = (val);                                              \
        }                                                                      \
    }

#endif /* CMC_EML_ERROR_H_INCLUDED */
