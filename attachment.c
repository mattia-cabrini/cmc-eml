/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "attachment.h"
#include "base64.h"
#include "error.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void att_set_init(att_set_p A)
{
    memset(A, 0, sizeof(A->attachments));
    A->count      = 0;
    A->body_index = -1;
}

int att_set_init_by_args(att_set_p A, int argc, char** argv)
{
    int cur;
    int ret;

    att_set_init(A);

    for (cur = 0; cur < argc && A->count < MAX_ATTACHMENTS; ++cur)
    {
        if (strcmp(argv[cur], "-a") != 0 && strcmp(argv[cur], "--attach") != 0)
            continue; /* Not an header */

        if (argc <= cur + 3)
        {
            sprintf(
                error_message,
                "Not enough parameters. #%d introduces an attachment, but no "
                "mime-type, file name and path is provided.",
                cur
            );
            return FATAL_PARAM;
        }

        ret = att_set_add(A, argv[cur + 1], argv[cur + 2], argv[cur + 3]);
        if (ret)
            return ret;
    }

    return OK;
}

int att_init(att_p A, char* mime, char* filename, char* path)
{
    A->fd = -1;
    STRCPY_OR_TOOLONG(A->path, path, sizeof(A->path), "Path too long");
    STRCPY_OR_TOOLONG(A->mime, mime, sizeof(A->mime), "Mime-Type too long");
    STRCPY_OR_TOOLONG(
        A->filename, filename, sizeof(A->filename), "Filename too long"
    );

    return OK;
}

int att_init_fd(att_p A, char* mime, char* filename, int fd)
{
    STRCPY_OR_TOOLONG(A->mime, mime, sizeof(A->mime), "Mime-Type too long");
    STRCPY_OR_TOOLONG(
        A->filename, filename, sizeof(A->filename), "Filename too long"
    );
    A->fd = fd;

    return OK;
}

int att_print(att_p A, int fd, const char* boundary, int body)
{
    int ret = OK;

    writeev(fd, "Content-Type: ", A->mime, "; charset=UTF-8\n", NULL);

    if (!body)
    {
        writeev(
            fd,
            "Content-Disposition: attachment; filename=\"",
            A->filename,
            "\"\n",
            NULL
        );
    }

    writeev(fd, "Content-Transfer-Encoding: base64\n\n", NULL);

    if (A->fd == -1)
    {
        A->fd = open(A->path, O_RDWR, 0444);
        sprintf(error_message, "Could not open file %s", A->path);
        assert(A->fd >= 0, A->fd + ERRNO_SPLIT, error_message);

        ret   = base64_file_to_file(A->fd, fd, 80);

        ret   = close(A->fd);
        A->fd = -1;
        assert(ret == 0, ret + ERRNO_SPLIT, "Could not close file");
    }
    else
    {
        ret = base64_file_to_file(A->fd, fd, 80);
    }

    return_iferr(ret);

    writeev(fd, "\n\n--------------", boundary, "\n", NULL);

    return OK;
}

int att_set_add(att_set_p A, char* mime, char* filename, char* path)
{
    int ret = OK;

    ret     = att_init(&A->attachments[A->count], mime, filename, path);

    if (ret == OK)
        ++A->count;

    return OK;
}

int att_set_add_fd(att_set_p A, char* mime, char* filename, int fd)
{
    int ret = OK;

    ret     = att_init_fd(&A->attachments[A->count], mime, filename, fd);

    if (ret == OK)
        ++A->count;

    return OK;
}

void att_set_set_body_index(att_set_p A)
{
    assert(A->count > 0, FATAL_LOGIC, "att_set_set_body_index: count = 0");

    A->body_index = A->count - 1;
}

int att_set_print(att_set_p A, int fd, char* boundary)
{
    int cur;
    int ret = OK;

    writeev(fd, "--------------", boundary, "\n", NULL);

    if (A->body_index > 0)
    {
        assert(
            A->body_index < A->count,
            FATAL_LOGIC,
            "att_set_print: body index out of bound"
        );
        ret = att_print(A->attachments + A->body_index, fd, boundary, 1);
    }

    for (cur = 0; ret == OK && cur < A->count; ++cur)
        if (cur != A->body_index)
            ret = att_print(A->attachments + cur, fd, boundary, 0);

    return ret;
}
