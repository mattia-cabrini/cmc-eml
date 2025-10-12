/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "attachment.h"
#include "base64.h"
#include "bigstring.h"
#include "error.h"
#include "header.h"
#include "util.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>

void get_rand_string(char*, size_t);

int main(int argc, char** argv)
{
    int                     ret;
    struct eml_header_set_t S;
    struct att_set_t        A;

    /* Main content */
    struct bigstring_t content;
    char               content_length[16];

    /* Boundary */
    char raw_boundary[52];
    char boundary_header[93];

#ifdef DEBUG
    base64_test_ALPHABET();
#endif

    ret = eml_header_set_init_by_args(&S, argc, argv);
    assert(ret == OK, ret, error_message);

    ret = att_set_init_by_args(&A, argc, argv);
    assert(ret == OK, ret, error_message);

    if (A.count == 0)
    {
        ret = bigstring_init(&content);
        assert(ret == OK, ret, error_message);

        ret = bigstring_append_file(&content, 0);
        assert(ret == OK, ret, error_message);

        sprintf(content_length, "%ld", content.cur);
        eml_header_set_add(&S, "Content-Length", content_length);

        eml_header_set_print(&S, 1);
        writeev(1, content.buf, NULL);

        bigstring_free(&content);
    }
    else
    {
        get_rand_string(raw_boundary, sizeof(raw_boundary));
        strcpy(boundary_header, "multipart/mixed; boundary=\"------------");
        memcpy(boundary_header + 39, raw_boundary, sizeof(raw_boundary));
        boundary_header[91] = '"';
        boundary_header[92] = '\0';

        eml_header_set_add(&S, "Content-Type", boundary_header);
        eml_header_set_add(&S, "MIME-Version", "1.0");

        eml_header_set_print(&S, 1);
        writeev(1, "This is a multi-part message in MIME format.\n", NULL);

        att_set_add_fd(&A, "text/plain", "", 0);
        att_set_set_body_index(&A);

        att_set_print(&A, 1, raw_boundary);
    }

    if (ret)
        fprintf(stderr, "%s\n", error_message);

    return ret;
}

void get_rand_string(char* str, size_t n)
{
    FILE*  fp;
    size_t rr;

    fp = fopen("/dev/urandom", "r");
    assert(fp != NULL, FATAL_SIGSEGV, "Could not open /dev/urandom");

    rr = fread(str, 1, n, fp);
    assert(rr > 0, FATAL_SIGSEGV, "Could not read /dev/urandom");
    fclose(fp);

    while (n--)
        str[n] = (char)('a' + (unsigned)str[n] % 26);
}
