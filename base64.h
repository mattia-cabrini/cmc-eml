/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_BASE64_H_INCLUDED
#define CMC_EML_BASE64_H_INCLUDED

#include "io.h"

extern int base64_file_to_file(file_p in, file_p out, int line_length);

#ifdef DEBUG
extern void base64_test_ALPHABET(void);
#endif

#endif /* CMC_EML_BASE64_H_INCLUDED */
