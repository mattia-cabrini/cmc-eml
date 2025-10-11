/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "error.h"
#include "header.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    int                     ret;
    struct eml_header_set_t S;

    ret = eml_header_set_init_by_args(&S, argc, argv);

    if (ret == OK)
    {
        eml_header_set_print(&S, stdout);
    }

    if (ret)
        fprintf(stderr, "%s\n", error_message);

    return ret;
}
