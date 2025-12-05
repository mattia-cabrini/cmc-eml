/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "header.h"
#include "error.h"
#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void eml_header_init(eml_header_p H)
{
    H->key[0]   = '\0';
    H->value[0] = '\0';
}

void eml_header_print(eml_header_p H, file_p F)
{
    file_write_strv(F, H->key, ": ", H->value, "\r\n", NULL);
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
    if (value == NULL)
        value = "";

    if (S->count == MAX_HEADERS)
    {
        strncpy(error_message, "too many headers", MAX_ERROR_SIZE);
        return BUFFER_FULL;
    }

    STRCPY_OR_TOOLONG(
        S->H[S->count].key, key, sizeof(S->H[S->count].key), "key too long"
    )

    STRCPY_OR_TOOLONG(
        S->H[S->count].value,
        value,
        sizeof(S->H[S->count].value),
        "value too long"
    )

    ++S->count;

    return OK;
}

int eml_header_set_add_by_command(eml_header_set_p S, const int* command)
{
    int res = OK;

    struct comm_t key_c;
    struct comm_t value_c;

    switch ((res = comm_get(command, "key", &key_c)))
    {
    case OK:
        switch (comm_get(command, "value", &value_c))
        {
        case NOT_FOUND:
            eml_header_set_add(S, key_c.value, "");
            break;
        case OK:
            eml_header_set_add(S, key_c.value, value_c.value);
            break;
        }
        break;

    case NOT_FOUND:
        strncpy(
            error_message,
            "eml_header_set_add_by_command: no key provided",
            MAX_ERROR_SIZE
        );
        return NOT_FOUND;

    default:
        return res;
    }

    return res;
}

void eml_header_set_copy(eml_header_set_p dst, eml_header_set_p src)
{
    memcpy(dst, src, sizeof(struct eml_header_set_t));
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

    file_write_strv(F, "\r\n", NULL);
}

int eml_header_set_addv(eml_header_set_p S, const char* key, ...)
{
    int     ret;
    va_list args;

    if (S->count == MAX_HEADERS)
    {
        strncpy(error_message, "too many headers", MAX_ERROR_SIZE);
        return BUFFER_FULL;
    }

    STRCPY_OR_TOOLONG(
        S->H[S->count].key, key, sizeof(S->H[S->count].key), "key too long"
    )

    va_start(args, key);
    ret = strnappendvv(S->H[S->count].value, MAX_HEADER_VALUE_SIZE, args);
    va_end(args);

    if (ret < 0)
        return STRING_TOO_LONG;

    ++S->count;

    return OK;
}
