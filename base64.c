/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "base64.h"
#include "error.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static const char ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#ifdef DEBUG
#include <stdlib.h>

void base64_test_ALPHABET(void)
{
    char        ch;
    const char* ab = ALPHABET;

    for (ch = 'A'; ch <= 'Z'; ++ch)
    {
        if (*ab != ch)
        {
            fprintf(stderr, "%c-%c\n", *ab, ch);
            exit(-1);
        }

        ++ab;
    }

    for (ch = 'a'; ch <= 'z'; ++ch)
    {
        if (*ab != ch)
        {
            fprintf(stderr, "%c-%c\n", *ab, ch);
            exit(-1);
        }

        ++ab;
    }

    for (ch = '0'; ch <= '9'; ++ch)
    {
        if (*ab != ch)
        {
            fprintf(stderr, "%c-%c\n", *ab, ch);
            exit(-1);
        }

        ++ab;
    }

    if (*ab != '+')
    {
        fprintf(stderr, "%c-+\n", *ab);
        exit(-1);
    }
    ++ab;

    if (*ab != '/')
    {
        fprintf(stderr, "%c-/\n", *ab);
        exit(-1);
    }
    ++ab;

    fprintf(stderr, "DEBUG\t base64 alphabet OK\n");

    return;
}
#endif

void base64_encode_three(char* dst, char* three, unsigned int size)
{
    int n6 = 0;

    assert(dst != NULL, FATAL_SIGSEGV, "base64_encode_three: dst ¬init");
    assert(three != NULL, FATAL_SIGSEGV, "base64_encode_three: three ¬init");
    assert(size && size < 4, FATAL_SIGSEGV, "base64_encode_three: dst ¬init");

    n6     = (three[0] & 0xFC) >> 2;
    dst[0] = ALPHABET[n6];

    n6     = ((three[0] & 0x3) << 4) | ((three[1] & 0xF0) >> 4);
    dst[1] = ALPHABET[n6];

    switch (size)
    {
    case 1:
        dst[2] = dst[3] = '=';
        break;
    case 2:
        n6     = (((three[1] & 0xF) << 2)) | ((three[2] & 0xC0) >> 6);
        dst[2] = ALPHABET[n6];
        dst[3] = '=';
        break;
    case 3:
        n6     = ((three[1] & 0xF) << 2) | ((three[2] & 0xC0) >> 6);
        dst[2] = ALPHABET[n6];

        n6     = (three[2] & 0x3F);
        dst[3] = ALPHABET[n6];
        break;
    }
}

int base64_file_to_file(file_p in, file_p out, int line_length)
{
#define cmc_base64_err_outfile "base64_file_to_file: could not print on outfile"
    char buf[4];
    char three[3]         = {0};

    ssize_t rw_bytes      = 0;
    int     current_ll    = 4;
    int     to_print_part = 0;

    struct rbuffer_t file_in;
    struct wbuffer_t file_out;

    int res;

    rbuffer_init(&file_in, in);
    wbuffer_init(&file_out, out);

    if (file_isreg(in))
    {
        res = file_seek(in, 0, SEEK_SET);
        assert(res == OK, res, "base64_file_to_file: seek set");
    }

    while ((rw_bytes = rbuffer_read(&file_in, three, sizeof(three))) > 0)
    {
        base64_encode_three(buf, three, (unsigned int)rw_bytes);

        if (current_ll + 4 > line_length)
        {
            to_print_part = (size_t)(line_length - current_ll);

            if (to_print_part > 0)
                wbuffer_put(&file_out, buf, (size_t)to_print_part);

            current_ll = (int)sizeof(buf) - to_print_part;
            wbuffer_put(&file_out, "\r\n", 2 * sizeof(char));
            wbuffer_put(&file_out, buf + to_print_part, (size_t)current_ll);
        }
        else
        {
            wbuffer_put(&file_out, buf, sizeof(buf));
        }

        three[0] = three[1] = three[2] = '\0';
        current_ll += 4;
    }

    wbuffer_flush(&file_out);

    return OK;
}
