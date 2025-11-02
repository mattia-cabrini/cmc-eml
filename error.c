/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "error.h"
#include "util.h"

#include <stdio.h>

char error_message[MAX_ERROR_SIZE];

void error_setgpg(gpgme_error_t err)
{
    strnappendv(
        error_message,
        MAX_ERROR_SIZE,
        gpgme_strsource(err),
        gpgme_strerror(err),
        NULL
    );

#ifdef DEBUG
    fprintf(
        stderr,
        "DEBUG error_setgpg: constructed error message: %s",
        error_message
    );
#endif
}
