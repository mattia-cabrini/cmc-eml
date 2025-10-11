/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_FATAL_H_INCLUDED
#define CMC_EML_FATAL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

/* [0, 1000]
 * OK and FATAL(s)
 */
#define OK 0
#define FATAL_PARAM 2
#define FATAL_SIGSEGV 3

/* (1e+3, 1e+6]
 * ¬FATAL ERROR(s)
 */
#define BUFFER_FULL 1001
#define STRING_TOO_LONG 1002

/* For errors such as x > 1e+6, the acctual error is errno = x - 10e+6 */
#define ERRNO_SPLIT 1000000

#define MAX_ERROR_SIZE 1024
extern char error_message[];

#define assert(cond, code, msg)                                                \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            {                                                                  \
                fprintf(stderr, "%s\n", msg);                                  \
                exit(code);                                                    \
            }                                                                  \
        }                                                                      \
    }

#endif /* CMC_EML_FATAL_H_INCLUDED */
