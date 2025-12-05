/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_COMM_H_INCLUDED
#define CMC_EML_COMM_H_INCLUDED

#include "io.h"

typedef struct comm_t
{
    char* key;
    char* value;
}* comm_p;

/**
 * Parameters:
 * - file_p F: file to read;
 * - int* arena: memory allocated to store key-value pairs;
 * - int n: number of integers that can be stored in the arena.
 */
extern int comm_next(file_p F, int* arena, int n);

/**
 * Search for a key-value in the arena.
 *
 * If key-value is found, COMM is set with key and value. COMM members MUST NOT
 * be freed.
 *
 * Return:
 * - OK, if the key-value is found
 * - NOT_FOUND, otherwise.
 *
 * Warning:
 * This function assumes that the arena is well formatted, thus it does not
 * check alignement.
 */
extern int comm_get(const int* arena, const char* key, comm_p COMM);

/**
 * Dump an arena on stderr
 */
extern void comm_dump(int* arena);

#endif /* CMC_EML_COMM_H_INCLUDED */
