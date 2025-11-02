/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#define _GNU_SOURCE

#include "feat.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "attachment.h"
#include "base64.h"
#include "bigstring.h"
#include "error.h"
#include "header.h"
#include "io.h"
#include "sign.h"
#include "util.h"

void get_rand_string(char*, size_t);
void print_clear_eml(
    file_p F, eml_header_set_p S, att_set_p A, file_p msg, size_t* size
);
void print_clear_eml_na(eml_header_set_p S, file_p msg, file_p out);
void print_clear_eml_a(eml_header_set_p S, att_set_p A, file_p msg, file_p out);

int main(int argc, char** argv)
{
    int                     ret;
    struct eml_header_set_t S;
    struct att_set_t        A;
    struct sign_spec_t      SIGN;

    struct file_t base_message;
    struct file_t stdout_f;
    struct file_t clear_eml;
    struct file_t enc_eml;

    size_t enc_eml_size   = 0;
    size_t clear_eml_size = 0;

    srand((unsigned int)time(NULL));
    file_set_fd(&base_message, 0);
    file_set_fd(&stdout_f, 1);

#ifdef DEBUG
    base64_test_ALPHABET();
#endif

    ret = eml_header_set_init_by_args(&S, argc, argv);
    assert(ret == OK, ret, error_message);

    ret = att_set_init_by_args(&A, argc, argv);
    assert(ret == OK, ret, "main: att_set_init_by_args");

    ret = sign_spec_init_by_args(&SIGN, argc, argv);
    assert(ret == OK, ret, "main: sign_spec_init_by_args");

    print_clear_eml(&clear_eml, &S, &A, &base_message, &clear_eml_size);

    if (SIGN.sign)
    {
        sign_to_file(&enc_eml, &clear_eml, &enc_eml_size, SIGN.key);
        ret = file_copy(&stdout_f, &enc_eml);
        assert(ret == OK, ret, "main: file copy");

        exit(10);
    }
    else
    {
        ret = file_copy(&stdout_f, &clear_eml);
        assert(ret == OK, ret, "main: file copy");
    }

    if (ret)
        fprintf(stderr, "%s\n", error_message);

    file_close(&clear_eml);

    return ret;
}

void get_rand_string(char* str, size_t n)
{
    while (n--)
        str[n] = (char)('a' + ((unsigned)rand()) % 26);
}

void print_clear_eml(
    file_p F, eml_header_set_p S, att_set_p A, file_p msg, size_t* size
)
{
    int   res;
    off_t maybe_size;

    assert(size != NULL, FATAL_LOGIC, "print_clear_eml: illegal size");

    res = file_open_tmp(F);
    assert(res == 0, res, "print_clear_eml: new tmp");

    if (A->count == 0)
        print_clear_eml_na(S, msg, F);
    else
        print_clear_eml_a(S, A, msg, F);

    maybe_size = file_cur(F);
    assert(maybe_size >= 0, ERRNO_SPLIT + errno, "print_clear_eml: file cur");

    *size = (size_t)maybe_size;
    res   = file_seek(F, 0, SEEK_SET);
    assert(res == OK, res, "print_clear_eml: file seek set");
}

void print_clear_eml_na(eml_header_set_p S, file_p msg, file_p out)
{
    int ret;

    /* Main content */
    struct bigstring_t content;
    char               content_length[16];

    ret = bigstring_init(&content);
    assert(ret == OK, ret, error_message);

    ret = bigstring_append_file(&content, msg);
    assert(ret == OK, ret, error_message);

    sprintf(content_length, "%ld", content.cur);
    eml_header_set_add(S, "Content-Length", content_length);

    eml_header_set_print(S, out);
    file_write_strv(out, content.buf, NULL);

    bigstring_free(&content);
}

void print_clear_eml_a(eml_header_set_p S, att_set_p A, file_p msg, file_p out)
{
    /* Boundary */
    char raw_boundary[53]; /* Including trailing NUL */
    char boundary_header[94];

    get_rand_string(raw_boundary, sizeof(raw_boundary) - sizeof(char));
    raw_boundary[52] = '\0';

    strcpy(boundary_header, "multipart/mixed;\n boundary=\"------------");
    memcpy(
        boundary_header + 40, raw_boundary, sizeof(raw_boundary) - sizeof(char)
    );
    boundary_header[92] = '"';
    boundary_header[93] = '\0';

    eml_header_set_add(S, "Content-Type", boundary_header);
    eml_header_set_add(S, "MIME-Version", "1.0");

    eml_header_set_print(S, out);
    file_write_strv(
        out, "This is a multi-part message in MIME format.\n", NULL
    );

    att_set_add_file(A, "text/plain", "", msg);
    att_set_set_body_index(A);

    att_set_print(A, out, raw_boundary);
}
