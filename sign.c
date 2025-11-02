/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#define _GNU_SOURCE

#include "sign.h"
#include "error.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <gpgme.h>
#include <string.h>
#include <unistd.h>

void sign_to_file(file_p outF, file_p inF, size_t* fsize, const char* key_name)
{
    gpgme_ctx_t   ctx;
    gpgme_data_t  in;
    gpgme_data_t  out;
    gpgme_key_t   key;
    gpgme_error_t err;
    char*         buf;
    int           oerrno;
    int           res;

    gpgme_check_version(NULL);

    err = gpgme_new(&ctx);
    if (err)
    {
        error_setgpg(err);
        assert(0, FATAL_GPGME, error_message);
    }

    gpgme_set_armor(ctx, 1);

    err = gpgme_get_key(ctx, key_name, &key, 1);
    if (err)
    {
        gpgme_release(ctx);
        error_setgpg(err);
        assert(0, FATAL_GPGME, error_message);
    }

    err = gpgme_signers_add(ctx, key);
    if (err)
    {
        gpgme_key_release(key);
        gpgme_release(ctx);
        error_setgpg(err);
        assert(0, FATAL_GPGME, error_message);
    }

    err = gpgme_data_new_from_fd(&in, inF->fd);
    if (err)
    {
        gpgme_key_release(key);
        gpgme_release(ctx);
        error_setgpg(err);
        assert(0, FATAL_GPGME, error_message);
    }

    err = gpgme_data_new(&out);
    if (gpgme_data_new(&out))
    {
        gpgme_data_release(in);
        gpgme_key_release(key);
        gpgme_release(ctx);
        error_setgpg(err);
        assert(0, FATAL_GPGME, error_message);
    }

    err = gpgme_op_sign(ctx, in, out, GPGME_SIG_MODE_NORMAL);
    if (err)
    {
        gpgme_data_release(in);
        gpgme_key_release(key);
        gpgme_data_release(out);
        gpgme_release(ctx);
        error_setgpg(err);
        assert(0, FATAL_GPGME, error_message);
    }

    gpgme_data_release(in);
    gpgme_key_release(key);

    buf = gpgme_data_release_and_get_mem(out, fsize);

    res = file_open_tmp(outF);
    if (res != OK)
    {
        oerrno = errno;
        gpgme_free(buf);
        gpgme_release(ctx);
        assert(0, oerrno + ERRNO_SPLIT, "");
    }

    file_write(outF, buf, *fsize);
    gpgme_free(buf);
    gpgme_release(ctx);
}

int sign_spec_init_by_args(sign_spec_p S, int argc, char** argv)
{
    int cur;

    S->sign = 0;

    for (cur = 1; cur < argc; ++cur)
    {
        if (!argv[cur][0])
            continue;
        if (!(argv[cur][0] == '-' &&
              ((argv[cur][1] == '-' && argv[cur][2] == 's') ||
               argv[cur][1] == 's')))
            continue;

        if (strcmp(argv[cur], "-s:k") == 0 ||
            strcmp(argv[cur], "--sign:key") == 0)
        {
            if (argc <= cur + 1)
            {
                sprintf(
                    error_message,
                    "Not enough parameters. #%d introduces a sign key but no "
                    "name is provided.",
                    cur
                );
                return FATAL_PARAM;
            }

            if (strlen(argv[cur + 1]) >= SIGN_MAX_KEY_SIZE)
            {
                sprintf(
                    error_message,
                    "Param #%d: name too long (max %d).",
                    cur + 1,
                    SIGN_MAX_KEY_SIZE
                );
                return FATAL_PARAM;
            }

            S->sign = 1;
            strcpy(S->key, argv[cur + 1]);
#ifdef DEBUG
            fprintf(stderr, "Sign:Key `%s`\n", S->key);
#endif
        }

        if (strcmp(argv[cur], "-s:p") == 0 ||
            strcmp(argv[cur], "--sign:preference") == 0)
        {
            if (argc <= cur + 1)
            {
                sprintf(
                    error_message,
                    "Not enough parameters. #%d introduces a sign preference "
                    "but no "
                    "preferecne is provided.",
                    cur
                );
                return FATAL_PARAM;
            }

            S->sign = 1;
            if (strcmp(argv[cur + 1], "no") == 0)
            {
                S->preference = SIGN_PREFER_NO;
            }
            else if (strcmp(argv[cur + 1], "mutual") == 0)
            {
                S->preference = SIGN_PREFER_MUTUAL;
            }
            else
            {
                sprintf(
                    error_message,
                    "Invalid parameter #%d: provided value is neither \"no\" "
                    "nor \"mutual\".",
                    cur + 1
                );
                return FATAL_PARAM;
            }
#ifdef DEBUG
            fprintf(stderr, "Sign:Preference `%d`\n", S->preference);
#endif
        }
    }

    return OK;
}
