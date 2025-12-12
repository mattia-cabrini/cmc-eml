/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#define _GNU_SOURCE

#include "feat.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "attachment.h"
#include "base64.h"
#include "comm.h"
#include "error.h"
#include "header.h"
#include "io.h"
#include "util.h"

#define MAIN_BODY_CLEAR "This is a multi-part message in MIME format.\r\n"
#define MAIN_BODY_SIGN                                                         \
    "This is an OpenPGP/MIME signed message (RFC 4880 and 3156)\r\n"

typedef struct global_data_t
{
    struct file_t stdin_f;

    struct eml_header_set_t S;
    struct att_set_t        A;
    /* struct sign_spec_t   SIGN; */
}* global_data_p;

static void global_data_init(global_data_p);

static int print_clear_eml_by_command(global_data_p GD, int* comm_arena);
static int print_signed_eml_by_command(global_data_p GD, int* comm_arena);

static int print_eml_a(
    eml_header_set_p S, att_set_p A, file_p out, const char* mimebody, int sign
);

int main(int argc, char** argv)
{
    int ret = OK;

    struct global_data_t GD;
    struct comm_t        command;
    int                  comm_arena[1024];

    (void)argc;
    (void)argv;

    srand((unsigned int)(time(NULL) + getpid()));
    global_data_init(&GD);

#ifdef DEBUG
    base64_test_ALPHABET();
#endif

    do
    {
        ret = comm_next(&GD.stdin_f, comm_arena, 1024);
        assert(ret == OK, ret, error_message);

        if (comm_get(comm_arena, "do", &command) == NOT_FOUND ||
            command.value == NULL)
        {
            assert(0, FATAL_LOGIC, "No `do` command provided.");
            continue;
        }

        STR_SWITCH_INIT()

        STR_IF_EQ(command.value, "quit") break;

        STR_IF_EQ(command.value, "add-header")
        ret = eml_header_set_add_by_command(&GD.S, comm_arena);

        STR_IF_EQ(command.value, "add-attachment")
        ret = att_set_add_by_command(&GD.A, comm_arena, 0);

        STR_IF_EQ(command.value, "set-body")
        ret = att_set_add_by_command(&GD.A, comm_arena, 1);

        STR_IF_EQ(command.value, "print-clear-eml")
        ret = print_clear_eml_by_command(&GD, comm_arena);

        STR_IF_EQ(command.value, "print-signed-eml")
        ret = print_signed_eml_by_command(&GD, comm_arena);

        STR_IF_EQ(command.value, "clear")
        global_data_init(&GD);

        STR_ELSE()
        {
            ret = FATAL_LOGIC;
            strnappendv(
                error_message,
                MAX_ERROR_SIZE,
                "`",
                command.value,
                "` not implemented, yet"
            );
        }

        assert(ret == OK, ret, error_message);
    } while (file_last_rb(&GD.stdin_f) > 0);

    return ret;
}

static int print_eml_a(
    eml_header_set_p S, att_set_p A, file_p out, const char* mainbody, int sign
)
{
    int res = OK;

    /* Boundary */
    char raw_boundary[53]; /* Including trailing NUL */
    char boundary_header[256];

    get_rand_string(raw_boundary, sizeof(raw_boundary) - sizeof(char));
    raw_boundary[52] = '\0';

    if (sign)
        strnappendv(
            boundary_header,
            sizeof(boundary_header),
            "multipart/signed"
            ";\r\n protocol=\"application/pgp-signature\""
            ";\r\n micalg=pgp-sha256"
            ";\r\n boundary=\"------------",
            raw_boundary,
            "\"",
            NULL
        );
    else
        strnappendv(
            boundary_header,
            sizeof(boundary_header),
            "multipart/mixed;\r\n boundary=\"------------",
            raw_boundary,
            "\"",
            NULL
        );

    res = eml_header_set_add(S, "Content-Type", boundary_header);
    return_iferr(res);

    res = eml_header_set_add(S, "MIME-Version", "1.0");
    return_iferr(res);

    res = eml_header_set_print(S, out);
    return_iferr(res);

    res = file_write_str(out, mainbody);
    return_iferr(res);

    res = att_set_print(A, out, raw_boundary);
    return_iferr(res);

    return res;
}

static void global_data_init(global_data_p GD)
{
    file_set_fd(&GD->stdin_f, STDIN_FILENO);

    eml_header_set_init(&GD->S);
    att_set_init(&GD->A);
}

static int print_clear_eml_by_command(global_data_p GD, int* comm_arena)
{
    int                     ret;
    struct comm_t           path_c;
    struct eml_header_set_t Scopy;
    struct file_t           out;

    if (comm_get(comm_arena, "path", &path_c) == NOT_FOUND ||
        path_c.value == NULL)
    {
        ret = ILLEGAL_FORMAT;
        strncpy(error_message, "no path provided", MAX_ERROR_SIZE);
        return ret;
    }

    ret = file_open(&out, path_c.value, O_RDWR | O_CREAT | O_TRUNC, 0644);
    return_iferr(ret);

    eml_header_set_copy(&Scopy, &GD->S);

    ret = print_eml_a(&Scopy, &GD->A, &out, MAIN_BODY_CLEAR, 0);

    file_close(&out);

    return ret;
}

static int print_signed_eml_by_command(global_data_p GD, int* comm_arena)
{
    int                     ret;
    struct comm_t           path_c;
    struct comm_t           clear_path_c;
    struct comm_t           sign_path_c;
    struct eml_header_set_t Scopy;
    struct file_t           out;

    if (comm_get(comm_arena, "clear-message", &clear_path_c) == NOT_FOUND ||
        clear_path_c.value == NULL)
    {
        ret = ILLEGAL_FORMAT;
        strncpy(error_message, "no clear-message provided", MAX_ERROR_SIZE);
        return ret;
    }

    if (comm_get(comm_arena, "signature", &sign_path_c) == NOT_FOUND ||
        sign_path_c.value == NULL)
    {
        ret = ILLEGAL_FORMAT;
        strncpy(error_message, "no signature provided", MAX_ERROR_SIZE);
        return ret;
    }

    if (comm_get(comm_arena, "path", &path_c) == NOT_FOUND ||
        path_c.value == NULL)
    {
        ret = ILLEGAL_FORMAT;
        strncpy(error_message, "no path provided", MAX_ERROR_SIZE);
        return ret;
    }

    ret = att_set_add(&GD->A, ATT_NOMIME, "", clear_path_c.value, ATT_FMT_7BIT);
    return_iferr(ret);

    ret = att_set_add(
        &GD->A,
        "application/pgp-signature",
        ATT_SIGNATURE_FILENAME,
        sign_path_c.value,
        ATT_FMT_7BIT
    );
    return_iferr(ret);

    ret = file_open(&out, path_c.value, O_RDWR | O_CREAT | O_TRUNC, 0644);
    return_iferr(ret);

    eml_header_set_copy(&Scopy, &GD->S);

    ret = print_eml_a(&Scopy, &GD->A, &out, MAIN_BODY_SIGN, 1);

    file_close(&out);

    return ret;
}
