/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "bigstring.h"
#include "error.h"
#include "header.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    int                     ret;
    struct eml_header_set_t S;
    struct bigstring_t      content;
    char                    content_length[16];

    ret = eml_header_set_init_by_args(&S, argc, argv);
    assert(ret == OK, ret, error_message);

    ret = bigstring_init(&content);
    assert(ret == OK, ret, error_message);

    ret = bigstring_append_file(&content, stdin);
    assert(ret == OK, ret, error_message);

    sprintf(content_length, "%ld", content.cur);
    eml_header_set_add(&S, "Content-Length", content_length);

    eml_header_set_print(&S, stdout);
    fprintf(stdout, "%s", content.buf);

    bigstring_free(&content);

    if (ret)
        fprintf(stderr, "%s\n", error_message);

    return ret;
}
