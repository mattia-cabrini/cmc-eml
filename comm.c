/* Copyright (c) 2025 Mattia Cabrini */
/* SPDX-License-Identifier: MIT      */

#include "comm.h"
#include "error.h"
#include "util.h"

#include <limits.h>

#define FSM_START -1
#define FSM_BLANK_FOR_KEY 0
#define FSM_READ_KEY 1
#define FSM_BLANK_FOR_VALUE 4
#define FSM_READ_VAL_ASCII 5
#define FSM_READ_VAL_VALUE 6
#define FSM_ESC 7

/**
 * A char is blank if it is in the set " \n\r\t"
 */
static int comm_is_blank(char c);

/**
 * A character is valid for a token if it is in the set a-z, A-Z, 0-9 or '-'
 */
static int comm_is_in_token_cs(char c);

struct char_block_t
{
    char* buf;
    int   sz;
    int cur; /* Next index to write to. Always set to a valid index. If `buf` is
                full, `cur` is kept set to `sz - 1`. The user shall not check if
                `buf` is full by comparing `cur` to `sz`, but rather in its own
                way. */
};

struct int_block_t
{
    int* buf;
    int  sz;
    int cur; /* Next index to write to. Always set to a valid index. If `buf` is
                full, `cur` is kept set to `sz - 1`. The user shall not check if
                `buf` is full by comparing `cur` to `sz`, but rather in its own
                way. */
};

typedef struct arena_builder_t
{
    /* Same arena, but treated as different type */
    struct int_block_t  i;
    struct char_block_t c;

    file_p F;

    int* str_init; /* Pointer to the integer before the string first char */
    int  rounder; /* If equal to sizeof int tells that next char index increment
                     shall also increase int buffer index; its value is in range
                     [1; sizeof(int)] */
    int state;    /* FSM state */
    int res;      /* Error condition */
    int done;     /* The machine is done formatting and shall not read more */
    int strlen;   /* Count current string chars; Reset when string is closed */

    char ch;
}* arena_builder_p;

#ifdef DEBUG
/** Dump */
void arena_builder_dump(arena_builder_p);
#endif

/** Initialize an arena builder with default values */
void arena_builder_init(arena_builder_p ARENA, file_p F, int* rawarena, int sz);

/** Increase char cur by one and int cur accordingly */
void arena_builder_incr(arena_builder_p ARENA);

/** Increase int cur by one and char cur accordingly */
void arena_builder_incr_int(arena_builder_p ARENA);

/** Read next char */
void arena_builder_read_next(arena_builder_p ARENA);

/** Append last read char */
void arena_builder_append(arena_builder_p ARENA);

/** Close the string and advance */
void arena_builder_close(arena_builder_p ARENA);

/* Exec States */

/** - Ignore any blank characters, including \r and \n;
 * - If a token character is provided, it gets added to the arena
 *   and the machine enters state FSM_READ_KEY;
 * - If a character is provided, that is not a token's nor a blank,
 *   the machine ends with result ILLEGAL_FORMAT. */
void arena_builder_exec_start(arena_builder_p);

/** - If \r or \n is provided, the machine exits succesfully;
 * - Ignore any blank character;
 * - If a token character is provided, it gets added to the arena
 *   and the machine enters state FSM_READ_KEY;
 * - If a character is provided, that is not a token's nor a blank,
 *   the machine ends with result ILLEGAL_FORMAT. */
void arena_builder_exec_blank_for_key(arena_builder_p);

/**
 * - If \r or \n is provided, the key, the value and the next key are closed
 *   and the machine exits succesfully;
 * - If a blank character is provided, the key and value closed and the
 *   machine state becomes FSM_BLANK_FOR_KEY;
 * - If '=' is provided the key is closed and the machine state
 *   becomes FSM_BLANMK_FOR_VALUE;
 * - If a token character is provided, it gets added to the string;
 * - If none of the above: the machine ends with result
 *   ILLEGAL_FORMAT. */
void arena_builder_exec_read_key(arena_builder_p);

/**
 * - If a blank character is provided (except for '\r' and '\n'), it
 *   gets ignored;
 * - If a token character is provided, it gets appended and the
 *   mahine state becomes FSM_READ_VAL_VALUE;
 * - If '"' is provided, the machine state becomes
 *   FSM_READ_VAL_ASCII;
 * - Otherwise the machine ends with state ILLEGAL_FORMAT. */
void arena_builder_exec_blank_for_value(arena_builder_p);

/**
 * - If '"' is provided, the machine closed and the machine state becomes
 *   FSM_BLANK_FOR_KEY;
 * - If '\\' is provided, the machine state becomes FSM_ESC.
 * - If '\r' or '\n' are provided, the machine exits with state ILLEGAL_FORMAT;
 * - Otherwise the character is added to the string. */
void arena_builder_exec_val_ascii(arena_builder_p);

/* - If \r or \n is provided, the string is closed and the machine
 *   exits succesfully;
 * - If a blank character is provided, the strign is closed and the
 *   machine state becomes FSM_BLANK_FOR_KEY;
 * - If a token character is provided, it gets added to the string;
 * - If none of the above: the machine ends with result
 *   ILLEGAL_FORMAT. */
void arena_builder_exec_val_value(arena_builder_p);

/* - If an escape character is provided (nrt\"), add to the string
 *   that special character and the machine state becomes (returns
 *   to) FSM_READ_VAL_ASCII;
 * - Otherwise the machine ends with status ILLEGAL_FORMAT. */
void arena_builder_exec_esc(arena_builder_p);

/* END Exex States */

void arena_builder_read_next(arena_builder_p ARENA)
{
    ssize_t rb;

    rb = file_read(ARENA->F, &ARENA->ch, sizeof(char));

    if (rb == 0) /* EOF */
    {
        ARENA->done = 1;

        switch (ARENA->state)
        {
        case FSM_START:
        case FSM_BLANK_FOR_KEY:
            break;

        case FSM_READ_KEY:
            /* Key */
            arena_builder_close(ARENA);
            return_void_iferr(ARENA->res);

            /* Value */
            arena_builder_close(ARENA);
            return_void_iferr(ARENA->res);
            break;

        case FSM_BLANK_FOR_VALUE:
        case FSM_READ_VAL_VALUE:
            /* Value */
            arena_builder_close(ARENA);
            return_void_iferr(ARENA->res);
            break;

        case FSM_READ_VAL_ASCII:
        case FSM_ESC:
            /* Error because '"' is expected */
            ARENA->res = ILLEGAL_FORMAT;
            return;

        default:
            strnappend(error_message, "Illegal state", MAX_ERROR_SIZE);
            ARENA->res = FATAL_LOGIC;
        }

        /* Terminator */
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);
    }
    else if (rb < 0)
    {
        strnappendv(
            error_message, MAX_ERROR_SIZE, "arena_builder_read_next", NULL
        );
        ARENA->res = ERRNO_SPLIT + errno;
    }
}

void arena_builder_incr(arena_builder_p ARENA)
{
    ARENA->c.cur += 1;

    if (ARENA->c.cur >= ARENA->c.sz)
    {
        ARENA->res = BUFFER_FULL;

        /* Cancel update: `cur` must always be set to a writeable index */
        ARENA->c.cur -= 1;
        return;
    }

    if (ARENA->rounder == sizeof_i(int))
    {
        ARENA->i.cur += 1;
        ARENA->rounder = 0;
    }

    ARENA->rounder += 1;
}

void arena_builder_incr_int(arena_builder_p ARENA)
{
    ARENA->c.cur += sizeof_i(int);
    ARENA->i.cur += 1;

    if (ARENA->c.cur >= ARENA->c.sz)
    {
        ARENA->res   = BUFFER_FULL;
        ARENA->c.cur = ARENA->c.sz - 1;
        ARENA->i.cur -= 1;
        return;
    }
}

void arena_builder_init(arena_builder_p ARENA, file_p F, int* rawarena, int n)
{
    if (n <= 0)
    {
        strncpy(error_message, "comm_next: arena size <= 0", MAX_ERROR_SIZE);
        ARENA->res = FATAL_LOGIC;
        return;
    }

    ARENA->i.buf = rawarena;
    ARENA->i.sz  = n;
    ARENA->i.cur = 0;

    if (n > INT_MAX / sizeof_i(int))
    {
        ARENA->res = ERRNO_SPLIT + EOVERFLOW;
        return;
    }
    ARENA->c.buf    = (char*)rawarena;
    ARENA->c.sz     = n * sizeof_i(int);
    ARENA->c.cur    = 0;

    ARENA->F        = F;
    ARENA->rounder  = 1;
    ARENA->state    = FSM_START;
    ARENA->res      = OK;
    ARENA->done     = 0;
    ARENA->strlen   = 0;

    ARENA->str_init = ARENA->i.buf;
    arena_builder_incr_int(ARENA);
}

void arena_builder_append(arena_builder_p ARENA)
{
    ARENA->c.buf[ARENA->c.cur] = ARENA->ch;
    ++ARENA->strlen;
    arena_builder_incr(ARENA);
    return_void_iferr(ARENA->res);
}

void arena_builder_close(arena_builder_p ARENA)
{
    if (ARENA->strlen > 0)
    {
        ARENA->ch = '\0';
        do
        {
            arena_builder_append(ARENA);
            return_void_iferr(ARENA->res);
        } while (ARENA->rounder != 1);
    }

    *ARENA->str_init =
        (int)((ARENA->i.buf + ARENA->i.cur) - ARENA->str_init - 1) *
        sizeof_i(int);

    ARENA->str_init = ARENA->i.buf + ARENA->i.cur;
    ARENA->strlen   = 0;
    arena_builder_incr_int(ARENA);
    return_void_iferr(ARENA->res);
}

void arena_builder_exec_start(arena_builder_p ARENA)
{
    if (ARENA->ch == '\n' || ARENA->ch == '\r')
        return;

    if (comm_is_blank(ARENA->ch))
    {
        ARENA->state = FSM_BLANK_FOR_KEY;
        return;
    }

    if (comm_is_in_token_cs(ARENA->ch))
    {
        ARENA->state = FSM_READ_KEY;
        arena_builder_append(ARENA);
        return;
    }

    ARENA->res = ILLEGAL_FORMAT;
}

void arena_builder_exec_blank_for_key(arena_builder_p ARENA)
{
    if (ARENA->ch == '\n' || ARENA->ch == '\r')
    {
        arena_builder_close(ARENA);
        ARENA->done = 1;
        return;
    }

    if (comm_is_blank(ARENA->ch))
        return;

    if (comm_is_in_token_cs(ARENA->ch))
    {
        ARENA->state = FSM_READ_KEY;
        arena_builder_append(ARENA);
        return;
    }

    ARENA->res = ILLEGAL_FORMAT;
}

void arena_builder_exec_read_key(arena_builder_p ARENA)
{
    if (ARENA->ch == '\r' || ARENA->ch == '\n')
    {
        /* Close key */
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        /* Close value */
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        /* Close next key */
        arena_builder_close(ARENA);

        ARENA->done = 1;
        return;
    }

    if (comm_is_in_token_cs(ARENA->ch))
    {
        ARENA->state = FSM_READ_KEY;
        arena_builder_append(ARENA);
        return;
    }

    if (comm_is_blank(ARENA->ch))
    {
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        ARENA->state = FSM_BLANK_FOR_KEY;
        return;
    }

    if (ARENA->ch == '=')
    {
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        ARENA->state = FSM_BLANK_FOR_VALUE;
        return;
    }

    ARENA->res = ILLEGAL_FORMAT;
}

void arena_builder_exec_blank_for_value(arena_builder_p ARENA)
{
    if (comm_is_blank(ARENA->ch) && ARENA->ch != '\r' && ARENA->ch != '\n')
        return;

    if (comm_is_in_token_cs(ARENA->ch))
    {
        arena_builder_append(ARENA);
        return_void_iferr(ARENA->res);

        ARENA->state = FSM_READ_VAL_VALUE;
        return;
    }

    if (ARENA->ch == '"')
    {
        ARENA->state = FSM_READ_VAL_ASCII;
        return;
    }

    ARENA->res = ILLEGAL_FORMAT;
}

void arena_builder_exec_val_ascii(arena_builder_p ARENA)
{
    if (ARENA->ch == '"')
    {
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        ARENA->state = FSM_BLANK_FOR_KEY;
        return;
    }

    if (ARENA->ch == '\\')
    {
        ARENA->state = FSM_ESC;
        return;
    }

    if (ARENA->ch == '\r' || ARENA->ch == '\n')
    {
        ARENA->res = ILLEGAL_FORMAT;
        return;
    }

    arena_builder_append(ARENA);
    return_void_iferr(ARENA->res);
}

void arena_builder_exec_val_value(arena_builder_p ARENA)
{
    if (comm_is_in_token_cs(ARENA->ch))
    {
        ARENA->state = FSM_READ_VAL_VALUE;
        arena_builder_append(ARENA);
        return_void_iferr(ARENA->res);
        return;
    }

    if (ARENA->ch == '\r' || ARENA->ch == '\n')
    {
        /* Close value */
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        /* Close next key */
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        ARENA->done = 1;
        return;
    }

    if (comm_is_blank(ARENA->ch))
    {
        arena_builder_close(ARENA);
        return_void_iferr(ARENA->res);

        ARENA->state = FSM_BLANK_FOR_KEY;
        return;
    }

    ARENA->res = ILLEGAL_FORMAT;
}

void arena_builder_exec_esc(arena_builder_p ARENA)
{
    ARENA->state = FSM_READ_VAL_ASCII;

    switch (ARENA->ch)
    {
    case 'n':
        ARENA->ch = '\n';
        break;
    case 'r':
        ARENA->ch = '\r';
        break;
    case 't':
        ARENA->ch = '\t';
        break;
    case '\\':
        ARENA->ch = '\\';
        break;
    case '"':
        ARENA->ch = '"';
        break;
    default:
        ARENA->res = ILLEGAL_FORMAT;
        return;
    }

    arena_builder_append(ARENA);
    return_void_iferr(ARENA->res);
}

/**
 * Parameters:
 * - F: file to read;
 * - arena: memory buffer into which the input is formatted and stored;
 * - n: size of arena in terms of number of storable integers.
 *
 * Errors:
 * - BUFFER_FULL: if arena is not big enough;
 * - FATAL_LOGIC: if a logical error happens;
 * - ILLEGAL_FORMAT: in an uinexpected token is encountered.
 *
 * Resulting arena will be a sequence of KV Blocks.
 *
 * A KV Block is made up by two contiguous SS Blocks.
 *
 * An SS Block is made up by:
 * - sizeof(int) bytes -> int value -> string length LEN in bytes, including all
 *   NUL-terminators;
 * - C-string with length LEN + NUL-terminators.
 *   - There is at least one NUL-terminator, in order to ensure that the block
 *     is a valid C-string.
 *   - There are enough NUL-terminatos to ensure that the byte next to the last
 *     NUL-terminator is addressable to as an int*.
 *
 * Example of valid SS Blocks:
 * ....Ciao sono io<NUL><NUL><NUL><NUL>
 * ^^^^
 * Int representation of 16.
 *
 * ....<NUL><NUL><NUL><NUL>
 * ^^^^
 * Int representation of 4.
 */
int comm_next(file_p F, int* rawarena, int n)
{
    struct arena_builder_t ARENA;

    arena_builder_init(&ARENA, F, rawarena, n);
    return_iferr(ARENA.res);

    for (; ARENA.res == OK && !ARENA.done;)
    {
        arena_builder_read_next(&ARENA);
        return_iferr(ARENA.res);

        if (ARENA.done)
            break;

        switch (ARENA.state)
        {
        case FSM_START:
            arena_builder_exec_start(&ARENA);
            break;

        case FSM_BLANK_FOR_KEY:
            arena_builder_exec_blank_for_key(&ARENA);
            break;

        case FSM_READ_KEY:
            arena_builder_exec_read_key(&ARENA);
            break;

        case FSM_BLANK_FOR_VALUE:
            arena_builder_exec_blank_for_value(&ARENA);
            break;

        case FSM_READ_VAL_ASCII:
            arena_builder_exec_val_ascii(&ARENA);
            break;

        case FSM_READ_VAL_VALUE:
            arena_builder_exec_val_value(&ARENA);
            break;

        case FSM_ESC:
            arena_builder_exec_esc(&ARENA);
            break;

        default:
            strnappend(error_message, "Illegal state", MAX_ERROR_SIZE);
            ARENA.res = FATAL_LOGIC;
        }
    }

    switch (ARENA.res)
    {
    case ILLEGAL_FORMAT:
        strncpy(error_message, "Illegal format", MAX_ERROR_SIZE);
        break;
    case BUFFER_FULL:
        strncpy(
            error_message,
            "Buffer full; Command arena too small",
            MAX_ERROR_SIZE
        );
        break;
    }

    return ARENA.res;
}

static int comm_is_blank(char c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static int comm_is_in_token_cs(char c)
{
    if (c >= 'a' && c <= 'z')
        return 1;

    if (c >= 'A' && c <= 'Z')
        return 1;

    if (c >= '0' && c <= '9')
        return 1;

    return c == '-';
}

int comm_get(const int* arena, const char* key, comm_p COMM)
{
    const int* cur_size;
    char*      cur_str;

    cur_size = arena;
    cur_str  = (char*)cur_size + sizeof_i(int);

    while (*cur_size > 0 && strcmp(cur_str, key) != 0)
    {
        /* Value */
        cur_size = ptr_add_bytes(int, cur_str, *cur_size);
        cur_str  = ptr_add_bytes(char, cur_size, sizeof(int));

        /* Key */
        cur_size = ptr_add_bytes(int, cur_str, *cur_size);
        cur_str  = ptr_add_bytes(char, cur_size, sizeof(int));
    }

    if (*cur_size)
    {
        COMM->key = cur_str;

        cur_size  = ptr_add_bytes(int, cur_size, *cur_size + sizeof_i(int));
        if (*cur_size > 0)
            COMM->value = ptr_add_bytes(char, cur_size, sizeof_i(int));
        else
            COMM->value = NULL;

        return OK;
    }

    return NOT_FOUND;
}

void comm_dump(int* arena)
{
    int*  cur_size;
    char* cur_str;

    cur_size = arena;
    cur_str  = (char*)cur_size + sizeof_i(int);

    while (*cur_size > 0)
    {
        fprintf(stderr, "Key Size: %d, Key: `%s`\n", *cur_size, cur_str);

        /* Value */
        cur_size = ptr_add_bytes(int, cur_str, *cur_size);
        cur_str  = ptr_add_bytes(char, cur_size, sizeof(int));

        if (*cur_size > 0)
            fprintf(
                stderr, "Value Size: %d, Value: `%s`\n", *cur_size, cur_str
            );
        else
            fprintf(stderr, "Value Size: %d\n", *cur_size);

        /* Key */
        cur_size = ptr_add_bytes(int, cur_str, *cur_size);
        cur_str  = ptr_add_bytes(char, cur_size, sizeof(int));
    }
}

#ifdef DEBUG
void arena_builder_dump(arena_builder_p ARENA)
{
    int  i; /* Char index */
    int  j /* Line size */;
    int  k;  /* Ind index */
    char ch; /* Char to print */

    fprintf(
        stderr,
        "Rounder:  %d\n"
        "State:    %d\n"
        "Res:      %d\n"
        "Done:     %d\n"
        "StrLen:    %d\n"
        "CH:       `%c` (%d)\n"
        "CHAR BUF: sz: %d, cur: %d\n"
        "INT  BUF: sz: %d, cur: %d\n",
        ARENA->rounder,
        ARENA->state,
        ARENA->res,
        ARENA->done,
        ARENA->strlen,
        (ARENA->ch >= 32 ? ARENA->ch : '.'),
        ARENA->ch,
        ARENA->c.sz,
        ARENA->c.cur,
        ARENA->i.sz,
        ARENA->i.cur
    );

    for (k = 0; k < ARENA->i.sz; ++k)
    {
        i = k * sizeof_i(int);

        fprintf(stderr, "%-4d: ", k);

        for (j = 0; i < ARENA->c.sz && j < sizeof_i(int); ++j)
        {
            ch = ARENA->c.buf[i];
            fprintf(stderr, "%c", ch >= 32 && ch < 127 ? ch : '.');
            ++i;
        }
        fprintf(stderr, " ");

        i -= sizeof_i(int);
        for (j = 0; i < ARENA->c.sz && j < sizeof_i(int); ++j)
        {
            ch = ARENA->c.buf[i];
            fprintf(stderr, "% -4d", ch);
            ++i;
        }

        fprintf(stderr, " -> %d\n", ARENA->i.buf[k]);
    }
}
#endif
