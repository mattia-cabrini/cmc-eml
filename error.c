/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "error.h"
#include "util.h"

#include <stdio.h>

char error_message[MAX_ERROR_SIZE];

void error_setgpg(gpgme_error_t err)
{
    int ret;

    ret = strnappendv(
        error_message,
        MAX_ERROR_SIZE,
        "GPGME error: SRC ",
        gpgme_strsource(err),
        "; ERR ",
        gpgme_strerror(err),
        NULL
    );

    if (ret < 0)
    {
        fprintf(stderr, "%s %s\n", gpgme_strsource(err), gpgme_strerror(err));
        assert(0, FATAL_LOGIC, "could not failsafe: buffer too small");
    }

#ifdef DEBUG
    fprintf(
        stderr,
        "DEBUG error_setgpg: constructed error message: %s\n",
        error_message
    );
#endif
}
