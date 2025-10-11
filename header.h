/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_HEADER_H_INCLUDED
#define CMC_EML_HEADER_H_INCLUDED

#define MAX_HEADER_KEY_SIZE 256
#define MAX_HEADER_VALUE_SIZE 10240
#define MAX_HEADERS 512

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
extern void eml_header_print(eml_header_p, FILE*);

extern void eml_header_set_init(eml_header_set_p);
extern int eml_header_set_init_by_args(eml_header_set_p, int argc, char** argv);
extern int
eml_header_set_add(eml_header_set_p, const char* key, const char* value);
extern void eml_header_set_print(eml_header_set_p, FILE*);

#endif /* CMC_EML_HEADER_H_INCLUDED */
