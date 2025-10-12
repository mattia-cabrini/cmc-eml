/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_ATTACHMENT_H_INCLUDED
#define CMC_EML_ATTACHMENT_H_INCLUDED

#define MAX_PATH_SIZE 256
#define MAX_MIME_SIZE 64
#define MAX_ATTACHMENTS 512

#include <stdio.h>

typedef struct att_t
{
    char path[MAX_PATH_SIZE];
    char mime[MAX_MIME_SIZE];
    char filename[MAX_PATH_SIZE];
    int  fd;
}* att_p;

typedef struct att_set_t
{
    struct att_t attachments[MAX_ATTACHMENTS];
    int          count;
    int          body_index;
}* att_set_p;

extern int att_init(att_p, char* mime, char* filename, char* path);
extern int att_init_fd(att_p, char* mime, char* filename, int fd);
extern int att_print(att_p, int, const char* boundary, int body);

extern void att_set_init(att_set_p);
extern int  att_set_init_by_args(att_set_p, int argc, char** argv);
extern int  att_set_add(att_set_p, char* mime, char* filename, char* path);
extern int  att_set_add_fd(att_set_p, char* mime, char* filename, int fd);
extern void att_set_set_body_index(att_set_p);
extern int  att_set_print(att_set_p, int, char* boundary);

#endif /* CMC_EML_ATTACHMENT_H_INCLUDED */
