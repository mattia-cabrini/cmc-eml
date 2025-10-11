/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "attachment.h"
#include "error.h"

#include <string.h>

void eml_attachment_set_init(eml_attachment_set_p A)
{
    memset(A, 0, sizeof(*A));
}

int eml_attachment_set_init_by_args(
    eml_attachment_set_p A, int argc, char** argv
)
{
    int cur;
    int ret;

    eml_attachment_set_init(A);

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

        ret = eml_attachment_set_add(
            A, argv[cur + 1], argv[cur + 2], argv[cur + 3]
        );
        if (ret)
            return ret;
    }

    return OK;
}

int eml_attachment_set_add(
    eml_attachment_set_p A, char* mime, char* filename, char* path
)
{
    size_t pathsize;
    size_t mimesize;
    size_t filenamesize;

    pathsize = (strlen(path) + 1) * sizeof(char);
    if (pathsize > sizeof(A->PATHS[A->count]))
    {
        strcpy(error_message, "Path too long");
        return STRING_TOO_LONG;
    }
    memcpy(A->PATHS[A->count], path, pathsize);

    mimesize = (strlen(mime) + 1) * sizeof(char);
    if (mimesize > sizeof(A->MIMEs[A->count]))
    {
        strcpy(error_message, "Mime-Type too long");
        return STRING_TOO_LONG;
    }
    memcpy(A->MIMEs[A->count], mime, mimesize);

    filenamesize = (strlen(filename) + 1) * sizeof(char);
    if (filenamesize > sizeof(A->FILENAMEs[A->count]))
    {
        strcpy(error_message, "Filename too long");
        return STRING_TOO_LONG;
    }
    memcpy(A->FILENAMEs[A->count], filename, filenamesize);

    ++A->count;
    return OK;
}
