/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_FATAL_H_INCLUDED
#define CMC_EML_FATAL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

#define OK 0
#define FATAL_PARAM 2

#define BUFFER_FULL 1001
#define STRING_TOO_LONG 1002

#define MAX_ERROR_SIZE 1024
extern char error_message[];

#endif /* CMC_EML_FATAL_H_INCLUDED */
