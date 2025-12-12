/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_ATTACHMENT_H_INCLUDED
#define CMC_EML_ATTACHMENT_H_INCLUDED

#define MAX_PATH_SIZE 256
#define MAX_MIME_SIZE 64
#define MAX_ATTACHMENTS 512

#include "comm.h"
#include "io.h"

/* Transfer Formats */
enum
{
    ATT_FMT_LBOUND = 0,
    ATT_FMT_BASE64 = 1,
    ATT_FMT_7BIT   = 2,
    ATT_FMT_UBOUND = 3
};

typedef struct att_t
{
    char   path[MAX_PATH_SIZE];
    char   mime[MAX_MIME_SIZE];
    char   filename[MAX_PATH_SIZE];
    int    fmt; /* Transfer format */
    file_p F;
}* att_p;

typedef struct att_set_t
{
    struct att_t attachments[MAX_ATTACHMENTS];
    int          count;
    int          body_index;
}* att_set_p;

extern const char* ATT_SIGNATURE_FILENAME;
extern const char* ATT_NOMIME;

extern int att_init(
    att_p, const char* mime, const char* filename, const char* path, int fmt
);
extern int att_print(att_p, file_p, const char* boundary, int body, int last);

extern void att_set_init(att_set_p);
extern int  att_set_add(
     att_set_p, const char* mime, const char* filename, const char* path, int fmt
 );
extern int  att_set_add_by_command(att_set_p, int* comm_arena, int is_body);
extern void att_set_set_body_index(att_set_p);
extern int  att_set_print(att_set_p, file_p, char* boundary);

#endif /* CMC_EML_ATTACHMENT_H_INCLUDED */
