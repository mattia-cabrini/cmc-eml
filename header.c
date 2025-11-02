/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "header.h"
#include "error.h"
#include "util.h"

#include <string.h>

void eml_header_init(eml_header_p H)
{
    H->key[0]   = '\0';
    H->value[0] = '\0';
}

void eml_header_print(eml_header_p H, file_p F)
{
    file_write_strv(F, H->key, ": ", H->value, "\n", NULL);
}

void eml_header_set_init(eml_header_set_p S)
{
    int cur;

    S->count = 0;
    for (cur = 0; cur < MAX_HEADERS; ++cur)
        eml_header_init(S->H + cur);
}

int eml_header_set_add(eml_header_set_p S, const char* key, const char* value)
{
    size_t keysize;
    size_t valuesize;

    if (S->count == MAX_HEADERS)
    {
        strncpy(error_message, "Header set is full", MAX_ERROR_SIZE);
        return BUFFER_FULL;
    }

    keysize = (strlen(key) + 1) * sizeof(char);
    if (keysize >= sizeof(S->H[S->count].key))
    {
        strncpy(error_message, "Key too long", MAX_ERROR_SIZE);
        return STRING_TOO_LONG;
    }
    memcpy(S->H[S->count].key, key, keysize);

    valuesize = (strlen(value) + 1) * sizeof(char);
    if (valuesize >= sizeof(S->H[S->count].value))
    {
        strncpy(error_message, "Value too long", MAX_ERROR_SIZE);
        return STRING_TOO_LONG;
    }
    memcpy(S->H[S->count].value, value, valuesize);
    ++S->count;

    return OK;
}

int eml_header_set_init_by_args(eml_header_set_p S, int argc, char** argv)
{
    int cur;
    int ret;

    eml_header_set_init(S);

    for (cur = 0; cur < argc && S->count < MAX_HEADERS; ++cur)
    {
        if (strcmp(argv[cur], "-h") != 0 && strcmp(argv[cur], "--header") != 0)
            continue; /* Not an header */

        if (argc <= cur + 2)
        {
            sprintf(
                error_message,
                "Not enough parameters. #%d introduces an header, but no "
                "key-value pair is provided.",
                cur
            );
            return FATAL_PARAM;
        }

        ret = eml_header_set_add(S, argv[cur + 1], argv[cur + 2]);
        if (ret)
            return ret;
    }

    return OK;
}

void eml_header_set_print(eml_header_set_p S, file_p F)
{
    int cur;

    for (cur = 0; cur < S->count; ++cur)
    {
        eml_header_print(S->H + cur, F);
    }

    file_write_strv(F, "\n", NULL);
}
