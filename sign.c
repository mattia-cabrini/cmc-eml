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

    S->sign          = 0;
    *S->keydata_path = '\0';

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

        if (strcmp(argv[cur], "-s:kd") == 0 ||
            strcmp(argv[cur], "--sign:keydata") == 0)
        {
            if (argc <= cur + 1)
            {
                sprintf(
                    error_message,
                    "Not enough parameters. #%d introduces sign keydata path "
                    "but no "
                    "path is provided.",
                    cur
                );
                return FATAL_PARAM;
            }

            if (strlen(argv[cur + 1]) * sizeof(char) >= sizeof(S->keydata_path))
            {
                strnappendv(
                    error_message,
                    MAX_ERROR_SIZE,
                    "keydata path too long (",
                    argv[cur + 1],
                    ")",
                    NULL
                );
                return FATAL_PARAM;
            }

            strcpy(S->keydata_path, argv[cur + 1]);

#ifdef DEBUG
            fprintf(stderr, "Sign:Keydata Path `%s`\n", S->keydata_path);
#endif
        }
    }

    return OK;
}

int sign_create_autocrypt_header(sign_spec_p SIGN, eml_header_set_p S)
{
    char          str[MAX_HEADER_VALUE_SIZE];
    int           res  = 0;
    int           left = 0;
    char          line[73];
    ssize_t       rb;
    struct file_t kf;
    int           resno;

    left += res = strnappendv(
        str + left, MAX_HEADER_VALUE_SIZE - left, "addr=", SIGN->key, NULL
    );

    if (res < 0)
    {
        strnappendv(
            error_message,
            MAX_ERROR_SIZE,
            "sign_create_autocrypt_header: could set autocrypt header: "
            "MAX_HEADER_VALUE_SIZE too small",
            NULL
        );
        return FATAL_SIGSEGV;
    }

    switch (SIGN->preference)
    {
    case SIGN_PREFER_NO:
        left += res = strnappendv(
            str + left,
            MAX_HEADER_VALUE_SIZE - left,
            " preference=nopreference ",
            NULL
        );
        break;
    case SIGN_PREFER_MUTUAL:
        left += res = strnappendv(
            str + left, MAX_HEADER_VALUE_SIZE - left, " preference=mutual", NULL
        );
        break;
    }

    if (res < 0)
    {
        strnappendv(
            error_message,
            MAX_ERROR_SIZE,
            "sign_create_autocrypt_header: could set autocrypt header: "
            "MAX_HEADER_VALUE_SIZE too small",
            NULL
        );
        return FATAL_SIGSEGV;
    }

    if (*SIGN->keydata_path)
    {
        left += res = strnappendv(
            str + left, MAX_HEADER_VALUE_SIZE - left, "; keydata=", NULL
        );

        if (res < 0)
        {
            strnappendv(
                error_message,
                MAX_ERROR_SIZE,
                "sign_create_autocrypt_header: could set autocrypt header: "
                "MAX_HEADER_VALUE_SIZE too small",
                NULL
            );
            return FATAL_SIGSEGV;
        }

        resno = file_open(&kf, SIGN->keydata_path, O_RDONLY, 0);
        if (resno)
        {
            strnappendv(
                error_message,
                MAX_ERROR_SIZE,
                "sign_create_autocrypt_header: could not open keydata file (",
                SIGN->keydata_path,
                ")",
                NULL
            );
            return resno;
        }

        rb = file_read(&kf, line, sizeof(line) - sizeof(char));
        while (rb > 0)
        {
            line[rb]    = '\0';
            left += res = strnappendv(
                str + left, MAX_HEADER_VALUE_SIZE - left, "\n ", line, NULL
            );

            if (res < 0)
            {
                strnappendv(
                    error_message,
                    MAX_ERROR_SIZE,
                    "sign_create_autocrypt_header: could set autocrypt header: "
                    "MAX_HEADER_VALUE_SIZE too small",
                    NULL
                );
                return FATAL_SIGSEGV;
            }

            rb = file_read(&kf, line, sizeof(line));
        }

        if (rb < 0)
        {
            strnappendv(
                error_message,
                MAX_ERROR_SIZE,
                "sign_create_autocrypt_header: could not read keydata file (",
                SIGN->keydata_path,
                ")",
                NULL
            );
            return ERRNO_SPLIT + errno;
        }

        file_close(&kf);
    }

    str[left] = '\0';
    eml_header_set_add(S, "Autocrypt", str);
    return OK;
}
