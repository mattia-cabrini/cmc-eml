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
    A->F = NULL;

    if (strcmp(mime, MIME_ENCVER) != 0 && (filename == NULL || path == NULL))
    {
        strncpy(
            error_message,
            "att_init: filename and path must be set",
            MAX_ERROR_SIZE
        );
        return FATAL_LOGIC;
    }

    if (path != NULL)
        STRCPY_OR_TOOLONG(A->path, path, sizeof(A->path), "Path too long")
    else
        *A->path = '\0';

    STRCPY_OR_TOOLONG(A->mime, mime, sizeof(A->mime), "Mime-Type too long")

    if (filename != NULL)
        STRCPY_OR_TOOLONG(
            A->filename, filename, sizeof(A->filename), "Filename too long"
        )
    else
        *A->filename = '\0';

    return OK;
}

int att_init_file(att_p A, char* mime, char* filename, file_p F)
{
    if (strcmp(mime, MIME_ENCVER) != 0 &&
        (filename == NULL || F == NULL || !file_is_init(F)))
    {
        strncpy(
            error_message,
            "att_init: filename and path must be set",
            MAX_ERROR_SIZE
        );
        return FATAL_LOGIC;
    }

    STRCPY_OR_TOOLONG(A->mime, mime, sizeof(A->mime), "Mime-Type too long");
    STRCPY_OR_TOOLONG(
        A->filename, filename, sizeof(A->filename), "Filename too long"
    );
    A->F = F;

    return OK;
}

int att_print(att_p A, file_p F, const char* boundary, int body)
{
    int           ret = OK;
    struct file_t tmp_file;

    file_write_strv(F, "Content-Type: ", A->mime, "; charset=UTF-8\n", NULL);

    if (!body && *A->filename)
    {
        file_write_strv(
            F,
            "Content-Disposition: attachment; filename=\"",
            A->filename,
            "\"\n",
            NULL
        );
    }

    if (strcmp(A->mime, MIME_ENCVER) == 0)
    {
        ret = file_write_strv(
            F,
            "Content-Transfer-Encoding: 7bit\n",
            "Content-Description: PGP/MIME version identification\n\n",
            "Version: 1",
            NULL
        );
    }
    else
    {
        file_write_strv(F, "Content-Transfer-Encoding: base64\n\n", NULL);

        if (!file_is_init(A->F))
        {
            ret = file_open(&tmp_file, A->path, O_RDWR, 0444);
            if (ret < 0)
                assert(0, ret, "att_print: open");

            ret = base64_file_to_file(&tmp_file, F, 80);
            file_close(&tmp_file);
        }
        else
        {
            ret = base64_file_to_file(A->F, F, 80);
        }
    }

    return_iferr(ret);

    file_write_strv(F, "\n\n--------------", boundary, "\n", NULL);

    return OK;
}

int att_set_add(att_set_p A, char* mime, char* filename, char* path)
{
    int ret = OK;

    ret     = att_init(&A->attachments[A->count], mime, filename, path);

    if (ret == OK)
        ++A->count;

    return ret;
}

int att_set_add_file(att_set_p A, char* mime, char* filename, file_p F)
{
    int ret = OK;

    ret     = att_init_file(&A->attachments[A->count], mime, filename, F);

    if (ret == OK)
        ++A->count;

    return ret;
}

void att_set_set_body_index(att_set_p A)
{
    assert(A->count > 0, FATAL_LOGIC, "att_set_set_body_index: count = 0");

    A->body_index = A->count - 1;
}

int att_set_print(att_set_p A, file_p F, char* boundary)
{
    int cur;
    int ret = OK;

    file_write_strv(F, "--------------", boundary, "\n", NULL);

    if (A->body_index > 0)
    {
        assert(
            A->body_index < A->count,
            FATAL_LOGIC,
            "att_set_print: body index out of bound"
        );
        ret = att_print(A->attachments + A->body_index, F, boundary, 1);
    }

    for (cur = 0; ret == OK && cur < A->count; ++cur)
        if (cur != A->body_index)
            ret = att_print(A->attachments + cur, F, boundary, 0);

    return ret;
}
