/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "attachment.h"
#include "base64.h"
#include "error.h"
#include "util.h"

#include <fcntl.h>
#include <string.h>

#ifdef DEBUG
static void att_dump(att_p A);
#endif

const char* ATT_SIGNATURE_FILENAME = "OpenPGP_signature.asc";
const char* ATT_NOMIME             = "NOMIME";

void att_set_init(att_set_p A)
{
    memset(A, 0, sizeof(A->attachments));
    A->count      = 0;
    A->body_index = -1;
}

int att_init(
    att_p A, const char* mime, const char* filename, const char* path, int fmt
)
{
    A->F   = NULL;
    A->fmt = fmt;

    if (mime == NULL)
    {
        strncpy(error_message, "att_init: empty mime", MAX_ERROR_SIZE);
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

int att_print(att_p A, file_p F, const char* boundary, int body, int last)
{
    int           ret = OK;
    int           nomime;
    struct file_t tmp_file;

#ifdef DEBUG
    if (body)
        fprintf(stderr, "Printing body...\n");
    else
        fprintf(stderr, "Printing attachment...\n");
    att_dump(A);
#endif

    nomime = strcmp(A->mime, ATT_NOMIME) == 0;
    if (!nomime)
    {
        ret = file_write_strv(F, "Content-Type: ", A->mime, NULL);
        return_iferr(ret);

        if (strcmp(A->mime, "application/pgp-signature") == 0)
            ret = file_write_strv(F, "; name=\"signature.asc\"\r\n", NULL);
        else
            ret = file_write_strv(F, "; charset=UTF-8\r\n", NULL);
        return_iferr(ret);

        if (!body && *A->filename)
        {
            if (strcmp(A->filename, ATT_SIGNATURE_FILENAME) == 0)
                ret = file_write_strv(
                    F,
                    "Content-Description: OpenPGP digital signature\r\n",
                    "Content-Disposition: attachment; filename=\"",
                    A->filename,
                    "\"\r\n",
                    NULL
                );
            else
                ret = file_write_strv(
                    F,
                    "Content-Disposition: attachment; filename=\"",
                    A->filename,
                    "\"\r\n",
                    NULL
                );

            return_iferr(ret);
        }

        switch (A->fmt)
        {
        case ATT_FMT_BASE64:
            ret = file_write_strv(
                F, "Content-Transfer-Encoding: base64\r\n", NULL
            );
            break;
        case ATT_FMT_7BIT:
            ret =
                file_write_strv(F, "Content-Transfer-Encoding: 7bit\r\n", NULL);
            break;
        }
        return_iferr(ret);

        ret = file_write_strv(F, "\r\n", NULL);
        return_iferr(ret);
    }

    if (!file_is_init(A->F))
    {
        ret = file_open(&tmp_file, A->path, O_RDONLY, 0444);
        if (ret != 0)
        {
            strnappendv(
                error_message,
                MAX_ERROR_SIZE,
                "att_print: open; ",
                A->path,
                NULL
            );
            assert(0, ret, error_message);
        }

        switch (A->fmt)
        {
        case ATT_FMT_BASE64:
            ret = base64_file_to_file(&tmp_file, F, 80);
            break;
        case ATT_FMT_7BIT:
            ret = file_copy(F, &tmp_file);
            break;
        }

        file_close(&tmp_file);
    }
    else
    {
        switch (A->fmt)
        {
        case ATT_FMT_BASE64:
            ret = base64_file_to_file(A->F, F, 80);
            break;
        case ATT_FMT_7BIT:
            ret = file_copy(F, A->F);
            break;
        }
    }

    return_iferr(ret);

    /* In case of body to sign, a trailing <CR><LF> is needed to clearly
     * separate the signed body and the boundary */
    if (nomime)
    {
        ret = file_write_strv(F, "\r\n", NULL);
        return_iferr(ret);
    }

    if (!nomime)
    {
        ret = file_write_str(F, "\r\n");
        return_iferr(ret);

        switch (A->fmt)
        {
        case ATT_FMT_7BIT:
            break;
        default:
            ret = file_write_str(F, "\r\n");
            return_iferr(ret);
            break;
        }
    }

    ret = file_write_strv(F, "--------------", boundary, NULL);
    return_iferr(ret);

    if (last)
    {
        ret = file_write_str(F, "--");
        return_iferr(ret);
    }

    ret = file_write_str(F, "\r\n");

    return ret;
}

int att_set_add(
    att_set_p   A,
    const char* mime,
    const char* filename,
    const char* path,
    int         fmt
)
{
    int ret = OK;

    if (A->count == MAX_ATTACHMENTS)
        return BUFFER_FULL;

    ret = att_init(&A->attachments[A->count], mime, filename, path, fmt);

#ifdef DEBUG
    att_dump(&A->attachments[A->count]);
#endif

    if (ret == OK)
        ++A->count;

    return ret;
}

int att_set_add_by_command(att_set_p A, int* COMM, int is_body)
{
    int ret = OK;
    int fmt = ATT_FMT_LBOUND;

    struct comm_t mime_c;
    struct comm_t filename_c;
    struct comm_t path_c;
    struct comm_t fmt_c;

    if (ret == OK)
        if (comm_get(COMM, "mime-type", &mime_c) == NOT_FOUND ||
            mime_c.value == NULL)
        {
            ret = NOT_FOUND;
            strncpy(error_message, "no mime-type provided", MAX_ERROR_SIZE);
        }

    if (ret == OK && !is_body)
        if (comm_get(COMM, "filename", &filename_c) == NOT_FOUND ||
            filename_c.value == NULL)
        {
            ret = NOT_FOUND;
            strncpy(error_message, "no filename provided", MAX_ERROR_SIZE);
        }

    if (ret == OK)
        if (comm_get(COMM, "path", &path_c) == NOT_FOUND ||
            path_c.value == NULL)
        {
            ret = NOT_FOUND;
            strncpy(error_message, "no path provided", MAX_ERROR_SIZE);
        }

    if (ret == OK)
    {
        if (comm_get(COMM, "fmt", &fmt_c) != NOT_FOUND)
        {
            if (fmt_c.value != NULL)
                fmt = fmt_c.value[0] - '0';

            if (fmt_c.value == NULL || strlen(fmt_c.value) != 1 ||
                fmt <= ATT_FMT_LBOUND || fmt >= ATT_FMT_UBOUND)
            {
                ret = ILLEGAL_FORMAT;
                strncpy(error_message, "invalid fmt provided", MAX_ERROR_SIZE);
            }
        }
        else
        {
            fmt = ATT_FMT_BASE64;
        }
    }

    if (ret == OK)
    {
        if (is_body)
            ret = att_set_add(A, mime_c.value, "", path_c.value, fmt);
        else
            ret = att_set_add(
                A, mime_c.value, filename_c.value, path_c.value, fmt
            );
    }

    if (ret == OK && is_body)
        att_set_set_body_index(A);

    return ret;
}

void att_set_set_body_index(att_set_p A)
{
    assert(A->count > 0, FATAL_LOGIC, "att_set_set_body_index: count = 0");

    A->body_index = A->count - 1;
#ifdef DEBUG
    fprintf(stderr, "Body Index set to: %d...\n", A->body_index);
    att_dump(&A->attachments[A->body_index]);
#endif
}

int att_set_print(att_set_p A, file_p F, char* boundary)
{
    int cur;
    int index_of_last_att;
    int ret = OK;

    ret     = file_write_strv(F, "--------------", boundary, "\r\n", NULL);
    return_iferr(ret);

    if (A->body_index >= 0)
    {
        assert(
            A->body_index < A->count,
            FATAL_LOGIC,
            "att_set_print: body index out of bound"
        );
        ret = att_print(
            A->attachments + A->body_index, F, boundary, 1, A->count == 1
        );
    }

    if (A->body_index == A->count - 1)
        index_of_last_att = A->body_index - 1;
    else
        index_of_last_att = A->count - 1;

    for (cur = 0; ret == OK && cur < A->count; ++cur)
        if (cur != A->body_index)
            ret = att_print(
                A->attachments + cur, F, boundary, 0, cur == index_of_last_att
            );

    return ret;
}

#ifdef DEBUG
static void att_dump(att_p A)
{
    fprintf(stderr, "Path:      `%s`\n", A->path);
    fprintf(stderr, "Mime-Type: `%s`\n", A->mime);
    fprintf(stderr, "Filename:  `%s`\n", A->filename);
    fprintf(stderr, "Fmt:       `%d`\n", A->fmt);
    fprintf(stderr, "Omitted File...\n");
}
#endif
