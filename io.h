/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#ifndef CMC_EML_IO_H_INCLUDED
#define CMC_EML_IO_H_INCLUDED

#include "feat.h"

#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct file_t
{
    int fd;
}* file_p;

extern void file_set_null(file_p F);
extern int  file_is_init(file_p F);

extern int  file_open(file_p F, const char* path, int flags, mode_t mode);
extern int  file_open_tmp(file_p F);
extern void file_set_fd(file_p, int fd);

extern ssize_t file_read(file_p F, char* buf, size_t max);

extern int file_write(file_p F, const char* buf, size_t count);
extern int file_write_str(file_p F, const char* str);
extern int file_write_strv(file_p F, ...);

extern int file_copy(file_p dst, file_p src);

extern int   file_seek(file_p F, off_t off, int whence);
extern off_t file_cur(file_p F);

extern int file_isreg(file_p F);

extern void file_close(file_p F);

#endif /* CMC_EML_IO_H_INCLUDED */
