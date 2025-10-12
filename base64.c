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

int base64_file_to_file(int fd, int out, int line_length)
{
#define cmc_base64_err_outfile "base64_file_to_file: could not print on outfile"
    char three[3] = {0};
    char buf[4];

    ssize_t rw_bytes      = 0;
    int     cur           = 0;
    int     n6            = 0;
    int     current_ll    = 4;
    int     to_print_part = 0;

    lseek(fd, 0, SEEK_SET);

    while ((rw_bytes = read(fd, three, sizeof(three))) != 0)
    {
        n6       = (three[0] & 0xFC) >> 2;
        buf[cur] = ALPHABET[n6];

        n6       = ((three[0] & 0x3) << 4) | ((three[1] & 0xF0) >> 4);
        buf[1]   = ALPHABET[n6];

        switch (rw_bytes)
        {
        case 1:
            buf[2] = buf[3] = '=';
            break;
        case 2:
            n6     = ((three[1] & 0xF << 2)) | ((three[2] & 0xC0) >> 6);
            buf[2] = ALPHABET[n6];
            buf[3] = '=';
            break;
        case 3:
            n6     = ((three[1] & 0xF) << 2) | ((three[2] & 0xC0) >> 6);
            buf[2] = ALPHABET[n6];

            n6     = (three[2] & 0x3F);
            buf[3] = ALPHABET[n6];
            break;
        }

        if (current_ll + 4 > line_length)
        {
            to_print_part = (size_t)(line_length - current_ll);

            if (to_print_part > 0)
                writee(out, buf, (size_t)to_print_part);

            current_ll = (int)sizeof(buf) - to_print_part;
            writee(out, "\n", sizeof(char));
            writee(out, buf, (size_t)current_ll);
        }
        else
        {
            writee(out, buf, sizeof(buf));
        }

        memset(three, 0, sizeof(three));
        current_ll += 4;
    }

    return OK;
}
