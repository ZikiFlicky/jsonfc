/* Copyright (c) 2021, Yuval Tasher (ziki.flicky@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * JSONFC, an easy to use and portable json parser for C.
 */

#include "types.h"
#include "parser.h"

#include <stdlib.h>
#include <errno.h>

/*
 * TODO: carriage return support
 * FIXME: deallocate everything on failure
 */

/* forwards */
static bool parse_as_value(struct JsonParser* parser, struct Value* out);

void parser_construct(struct JsonParser* const parser, char* const stream) {
    parser->stream = stream;
    parser->idx = 0;
    parser->line = 1;
    parser->column = 1;
    parser->head = malloc(sizeof(struct Value));
}

static bool parser_advance(struct JsonParser* const parser, const uint amount) {
    uint i;
    for (i = 0; i < amount; i++) {
        switch (CURRENT_CHAR(*parser)) {
        case '\0':
            return 0;
        case '\n':
            parser->line++;
            parser->column = 1;
            break;
        default:
            parser->column++;
            break;
        }
        parser->idx++;
    }
    return 1;
}

static bool parser_clean(struct JsonParser* const parser) {
    uint start;
    start = parser->idx;
    while (CURRENT_CHAR(*parser) == ' '  || CURRENT_CHAR(*parser) == '\n' ||
           CURRENT_CHAR(*parser) == '\t' || CURRENT_CHAR(*parser) == '\r')
        parser_advance(parser, 1);
    if (start == parser->idx)
        return 0; /* didn't clean anything */
    return 1;
}

static bool match(struct JsonParser* const parser, const char *const stream) {
    uint i;
    for (i = 0; stream[i]; i++)
        if (stream[i] != CHAR_AT(*parser, i))
            return 0;
    parser_advance(parser, i);
    return 1;
}

static bool parse_as_number(struct JsonParser* const parser, double* const out) {
    /* FIXME: this function shouldn't support 0x, 0o and 0b prefixes in number parsing */
    double number;
    char *new_start;
    errno = 0;
    number = strtod(&CURRENT_CHAR(*parser), &new_start);
    if ((new_start == &CURRENT_CHAR(*parser) && number == 0) || errno == ERANGE)
        return 0;
    parser_advance(parser, new_start - &CURRENT_CHAR(*parser));
    *out = number;
    return 1;
}

static bool parse_as_string(struct JsonParser* const parser, char** const out, const bool allow_escapes) {
    uint write_idx, len;
    write_idx = 0;
    len = 12;

    if (CURRENT_CHAR(*parser) != '"')
        return 0;

    parser_advance(parser, 1);
    *out = malloc(len * sizeof(char));

    while (CURRENT_CHAR(*parser) != '"') {
        if (write_idx >= len) {
            len += 8;
            *out = realloc(*out, len * sizeof(char));
            if (*out == NULL)
                return 0;
        }
        /* TODO: add unicode and unicode escape sequence support */
        if (CURRENT_CHAR(*parser) == '\\') {
            if (!allow_escapes)
                return 0;
            switch (CHAR_AT(*parser, 1)) {
            case '\\': (*out)[write_idx] = '\\'; break;
            case '/': (*out)[write_idx] = '/'; break;
            case '"': (*out)[write_idx] = '"'; break;
            case 'b': (*out)[write_idx] = '\b'; break;
            case 'f': (*out)[write_idx] = '\f'; break;
            case 'n': (*out)[write_idx] = '\n'; break;
            case 'r': (*out)[write_idx] = '\r'; break;
            case 't': (*out)[write_idx] = '\t'; break;
            default: return 0;
            }
            parser_advance(parser, 2);
            write_idx++;
            continue;
        }

        if (CURRENT_CHAR(*parser) == '\n')
            return 0;

        if (CURRENT_CHAR(*parser) == '\0')
            return 0;

        (*out)[write_idx] = CURRENT_CHAR(*parser);
        parser_advance(parser, 1);
        write_idx++;
    }
    parser_advance(parser, 1);
    *out = realloc(*out, write_idx * sizeof(char));
    return 1;
}

static bool parse_as_array(struct JsonParser* const parser, struct Array** const out) {
    struct Array *array;
    struct Value tmp_val;

    if (CURRENT_CHAR(*parser) != '[')
        return 0;

    parser_advance(parser, 1);
    array = malloc(sizeof(struct Array));
    array_construct(array);

    while (CURRENT_CHAR(*parser) != ']') {
        if (parser_clean(parser))
            continue;

        if (CURRENT_CHAR(*parser) == '\0')
            return 0;

        if (!parse_as_value(parser, &tmp_val))
            return 0;

        if (CURRENT_CHAR(*parser) == ',') {
            parser_advance(parser, 1);
            parser_clean(parser);
            if (CURRENT_CHAR(*parser) == '}')
                return 0;
        }
        array_push(array, &tmp_val);
    }

    parser_advance(parser, 1); /* advance '}' */
    *out = array;
    return 1;
}

static bool parse_as_object(struct JsonParser* const parser, struct Object **out) {
    struct Object *obj;
    char *tmp_key;
    struct Value tmp_val;

    if (CURRENT_CHAR(*parser) != '{')
        return 0;

    parser_advance(parser, 1);
    obj = malloc(sizeof(struct Object));
    object_construct(obj);

    while (CURRENT_CHAR(*parser) != '}') {
        if (parser_clean(parser))
            continue;

        if (CURRENT_CHAR(*parser) == '\0')
            return 0;

        if (!parse_as_string(parser, &tmp_key, 0))
            return 0;

        parser_clean(parser);

        if (CURRENT_CHAR(*parser) != ':')
            return 0;

        parser_advance(parser, 1);
        parser_clean(parser);

        if (!parse_as_value(parser, &tmp_val))
            return 0;

        object_set(obj, tmp_key, &tmp_val);
        parser_clean(parser);

        if (CURRENT_CHAR(*parser) == ',') {
            parser_advance(parser, 1);
            parser_clean(parser);
            if (CURRENT_CHAR(*parser) == '}')
                return 0;
        }
    }

    parser_advance(parser, 1);
    parser_clean(parser);
    *out = obj;
    return 1;
}

static bool parse_as_null(struct JsonParser* const parser) {
    if (match(parser, "null"))
        return 1;
    return 0;
}

static bool parse_as_bool(struct JsonParser* const parser, bool* const out) {
    if (match(parser, "true")) {
        *out = 1;
        return 1;
    }
    if (match(parser, "false")) {
        *out = 0;
        return 1;
    }
    return 0;
}

static bool parse_as_value(struct JsonParser* const parser, struct Value* const out) {
    struct Value tmp;
    parser_clean(parser);
    if (parse_as_number(parser, &out->as.number)) out->type = Number;
    else if (parse_as_string(parser, &out->as.string, 1)) out->type = String;
    else if (parse_as_array(parser, &out->as.array)) out->type = Array;
    else if (parse_as_object(parser, &out->as.object)) out->type = Object;
    else if (parse_as_null(parser)) out->type = Null;
    else if (parse_as_bool(parser, &out->as.bool_)) out->type = Bool;
    else return 0; /* failed to parse as anything :^( */
    parser_clean(parser);

    /* FIXME: this is a little repetitive */
    if (parse_as_number(parser, &tmp.as.number)) return 0;
    if (parse_as_string(parser, &tmp.as.string, 1)) return 0;
    if (parse_as_array(parser, &tmp.as.array)) return 0;
    if (parse_as_object(parser, &tmp.as.object)) return 0;
    if (parse_as_null(parser)) return 0;
    if (parse_as_bool(parser, &tmp.as.bool_)) return 0;
    return 1;
}

struct Value *parse(char* const stream) {
    struct JsonParser parser;
    parser_construct(&parser, stream);
    if (!parse_as_value(&parser, parser.head))
        return NULL;
    return parser.head;
}

void value_delete(struct Value* const value) {
    switch (value->type) {
    case Number:
    case Null:
    case Bool:
        break;
    case String:
        free(value->as.string);
        break;
    case Array:
        array_dealloc(value->as.array);
        break;
    case Object:
        object_dealloc(value->as.object);
        break;
    default:
        return;
    }
    value->type = 0;
    free(value);
}
