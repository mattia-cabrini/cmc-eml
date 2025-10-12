/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "util.h"
#include "error.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

void writee(int fd, char* buf, size_t sz)
{
    ssize_t res;
    res = write(fd, buf, sz);

    if ((size_t)res != sz)
    {
        sprintf(error_message, "write on %d failed", fd);
        assert((size_t)res == sz, FATAL_SIGSEGV, error_message);
    }
}

void writeev(int fd, ...)
{
    va_list args;
    char*   str;

    va_start(args, fd);

    while ((str = va_arg(args, char*)) != NULL)
        writee(fd, str, strlen(str) * sizeof(char));

    va_end(args);
}
