/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_ATTACHMENT_H_INCLUDED
#define CMC_EML_ATTACHMENT_H_INCLUDED

#define MAX_PATH_SIZE 256
#define MAX_ATTACHMENTS 512

#include <stdio.h>

typedef struct eml_attachment_set_t
{
    char PATHS[MAX_ATTACHMENTS][MAX_PATH_SIZE];
    char MIMEs[MAX_ATTACHMENTS][MAX_PATH_SIZE];
    char FILENAMEs[MAX_ATTACHMENTS][MAX_PATH_SIZE];
    int  count;
}* eml_attachment_set_p;

extern void eml_attachment_set_init(eml_attachment_set_p);
extern int
eml_attachment_set_init_by_args(eml_attachment_set_p, int argc, char** argv);
extern int eml_attachment_set_add(
    eml_attachment_set_p, char* mime, char* filename, char* path
);

#endif /* CMC_EML_ATTACHMENT_H_INCLUDED */
