/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_HEADER_H_INCLUDED
#define CMC_EML_HEADER_H_INCLUDED

#define MAX_HEADER_KEY_SIZE 64
#define MAX_HEADER_VALUE_SIZE (4096 - 64)
#define MAX_HEADERS 64

#include "comm.h"
#include "io.h"

#include <stdio.h>

typedef struct eml_header_t
{
    char key[MAX_HEADER_KEY_SIZE];
    char value[MAX_HEADER_VALUE_SIZE];
}* eml_header_p;

typedef struct eml_header_set_t
{
    struct eml_header_t H[MAX_HEADERS];
    int                 count;
}* eml_header_set_p;

extern void eml_header_init(eml_header_p);
extern void eml_header_print(eml_header_p, file_p F);

extern void eml_header_set_init(eml_header_set_p);
extern void eml_header_set_copy(eml_header_set_p dst, eml_header_set_p src);
extern int eml_header_set_init_by_args(eml_header_set_p, int argc, char** argv);
extern int eml_header_set_addv(eml_header_set_p, const char*, ...);
extern int
eml_header_set_add(eml_header_set_p, const char* key, const char* value);
extern int  eml_header_set_add_by_command(eml_header_set_p, const int* command);
extern void eml_header_set_print(eml_header_set_p, file_p F);

#endif /* CMC_EML_HEADER_H_INCLUDED */
