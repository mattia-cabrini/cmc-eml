/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "attachment.h"
#include "bigstring.h"
#include "error.h"
#include "header.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    int ret;
    int cur;

    struct eml_header_set_t     S;
    struct eml_attachment_set_t A;

    /* Main content */
    struct bigstring_t content;
    char               content_length[16];

    ret = eml_header_set_init_by_args(&S, argc, argv);
    assert(ret == OK, ret, error_message);

    ret = eml_attachment_set_init_by_args(&A, argc, argv);
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

    for (cur = 0; cur < A.count; ++cur)
    {
        fprintf(
            stderr,
            "Attachment skipped (not supported, yet) -- %s; %s; %s\n",
            A.MIMEs[cur],
            A.FILENAMEs[cur],
            A.PATHS[cur]
        );
    }

    if (ret)
        fprintf(stderr, "%s\n", error_message);

    return ret;
}
