/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_UTIL_H_INCLUDED
#define CMC_EML_UTIL_H_INCLUDED

#include <stddef.h>

extern void writee(int fd, char* buf, size_t);
extern void writeev(int fd, ...);

#endif /* CMC_EML_UTIL_H_INCLUDED */
