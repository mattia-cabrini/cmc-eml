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

int eml_header_print(eml_header_p H, file_p F)
{
    return file_write_strv(F, H->key, ": ", H->value, "\r\n", NULL);
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
    if (key == NULL)
    {
        strncpy(error_message, "key empty", MAX_ERROR_SIZE);
        return FATAL_LOGIC;
    }

    if (S->count == MAX_HEADERS)
    {
        strncpy(error_message, "too many headers", MAX_ERROR_SIZE);
        return BUFFER_FULL;
    }

    if (value == NULL)
        value = "";

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
            res = eml_header_set_add(S, key_c.value, "");
            break;
        case OK:
            res = eml_header_set_add(S, key_c.value, value_c.value);
            break;
        default:
            strncpy(
                error_message,
                "eml_header_set_add_by_command: unhandled comm_get error",
                MAX_ERROR_SIZE
            );
            res = FATAL_LOGIC;
        }
        break;

    case NOT_FOUND:
        strncpy(
            error_message,
            "eml_header_set_add_by_command: no key provided",
            MAX_ERROR_SIZE
        );
        res = NOT_FOUND;
        break;

    default:
        strncpy(
            error_message,
            "eml_header_set_add_by_command: unhandled comm_get error",
            MAX_ERROR_SIZE
        );
        res = FATAL_LOGIC;
    }

    return res;
}

void eml_header_set_copy(eml_header_set_p dst, eml_header_set_p src)
{
    memcpy(dst, src, sizeof(struct eml_header_set_t));
}

int eml_header_set_print(eml_header_set_p S, file_p F)
{
    int ret = OK;
    int cur;

    for (cur = 0; ret == OK && cur < S->count; ++cur)
        ret = eml_header_print(S->H + cur, F);

    if (ret == OK)
        ret = file_write_strv(F, "\r\n", NULL);

    return ret;
}
