/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_SIGN_H_INCLUDED
#define CMC_EML_SIGN_H_INCLUDED

#include "header.h"
#include "io.h"

#include <stdio.h>

#define SIGN_MAX_KEY_SIZE 256

enum
{
    SIGN_PREFER_NO     = 0,
    SIGN_PREFER_MUTUAL = 1
};

typedef struct sign_spec_t
{
    int  sign;
    char key[SIGN_MAX_KEY_SIZE];
    char keydata_path[256];
    char keypwd_path[256];
    int  preference;
}* sign_spec_p;

extern int  sign_spec_init_by_args(sign_spec_p S, int argc, char** argv);
extern void sign_to_file(
    sign_spec_p SIGN, file_p out, file_p in, size_t* fsize, const char* key_name
);
extern int sign_create_autocrypt_header(sign_spec_p SIGN, eml_header_set_p S);

#endif /* CMC_EML_SIGN_H_INCLUDED */
