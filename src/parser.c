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
#include <stdio.h>

/*
 * TODO: carriage return support
 * TODO: unicode support
 * FIXME: deallocate everything on failure
 */

/* set this to true if you want colored output */
bool json_print_colored = true;
bool json_print_double_quoted = false;
bool json_print_key_as_string = false;


/* forwards */
static bool parse_as_value(struct JsonParser* parser, struct Value* out);

void parser_construct(struct JsonParser* const parser, char* const stream) {
    parser->stream = stream;
    parser->idx = 0;
    parser->line = 1;
    parser->column = 1;
    parser->head = malloc(sizeof(struct Value));
}

static bool parser_advance(struct JsonParser* const parser, const size_t amount) {
    size_t i;
    for (i = 0; i < amount; i++) {
        switch (CURRENT_CHAR(*parser)) {
        case '\0':
            return false;
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
    return true;
}

static bool parser_clean(struct JsonParser* const parser) {
    size_t start;
    start = parser->idx;
    while (CURRENT_CHAR(*parser) == ' '  || CURRENT_CHAR(*parser) == '\n' ||
           CURRENT_CHAR(*parser) == '\t' || CURRENT_CHAR(*parser) == '\r')
        parser_advance(parser, 1);
    if (start == parser->idx)
        return false; /* didn't clean anything */
    return true;
}

static bool match(struct JsonParser* const parser, const char *const stream) {
    size_t i;
    for (i = 0; stream[i]; i++)
        if (stream[i] != CHAR_AT(*parser, i))
            return false;
    parser_advance(parser, i);
    return true;
}

static bool parse_as_number(struct JsonParser* const parser, double* const out) {
    /* FIXME: this function shouldn't support extended bases */
    double number;
    char *end;
    number = strtod(&CURRENT_CHAR(*parser), &end);
    if (end == &CURRENT_CHAR(*parser) && number == 0)
        return false;
    parser_advance(parser, end - &CURRENT_CHAR(*parser));
    *out = number;
    return true;
}

static bool parse_as_string(struct JsonParser* const parser, char** const out, const bool allow_escapes) {
    size_t write_idx, len;
    write_idx = 0;
    len = 12;

    if (CURRENT_CHAR(*parser) != '"')
        return false;

    parser_advance(parser, 1);
    *out = malloc(len * sizeof(char));

    if (*out == NULL)
        return false;

    while (CURRENT_CHAR(*parser) != '"') {
        if (write_idx >= len) {
            len += 8;
            *out = realloc(*out, len * sizeof(char));
            if (*out == NULL)
                return false;
        }
        if (CURRENT_CHAR(*parser) == '\\') {
            if (!allow_escapes)
                return false;
            switch (CHAR_AT(*parser, 1)) {
            case '\\': (*out)[write_idx] = '\\'; break;
            case '/': (*out)[write_idx] = '/'; break;
            case '"': (*out)[write_idx] = '"'; break;
            case 'b': (*out)[write_idx] = '\b'; break;
            case 'f': (*out)[write_idx] = '\f'; break;
            case 'n': (*out)[write_idx] = '\n'; break;
            case 'r': (*out)[write_idx] = '\r'; break;
            case 't': (*out)[write_idx] = '\t'; break;
            default: return false;
            }
            parser_advance(parser, 2);
            write_idx++;
            continue;
        }

        if (CURRENT_CHAR(*parser) == '\n')
            return false;

        if (CURRENT_CHAR(*parser) == '\0')
            return false;

        (*out)[write_idx] = CURRENT_CHAR(*parser);
        parser_advance(parser, 1);
        write_idx++;
    }
    parser_advance(parser, 1);
    *out = realloc(*out, write_idx * sizeof(char));
    return true;
}

static bool parse_as_array(struct JsonParser* const parser, struct Array** const out) {
    struct Array *array;
    struct Value tmp_val;

    if (CURRENT_CHAR(*parser) != '[')
        return false;

    parser_advance(parser, 1);
    array = malloc(sizeof(struct Array));

    if (array == NULL)
        return false;

    array_construct(array);

    while (CURRENT_CHAR(*parser) != ']') {
        if (parser_clean(parser))
            continue;

        if (CURRENT_CHAR(*parser) == '\0')
            return false;

        if (!parse_as_value(parser, &tmp_val))
            return false;

        if (CURRENT_CHAR(*parser) == ',') {
            parser_advance(parser, 1);
            parser_clean(parser);
            if (CURRENT_CHAR(*parser) == '}')
                return false;
        }
        array_push(array, &tmp_val);
    }

    parser_advance(parser, 1); /* advance '}' */
    *out = array;
    return true;
}

static bool parse_as_object(struct JsonParser* const parser, struct Object **out) {
    struct Object *obj;
    char *tmp_key;
    struct Value tmp_val;

    if (CURRENT_CHAR(*parser) != '{')
        return false;

    parser_advance(parser, 1);
    obj = malloc(sizeof(struct Object));

    if (obj == NULL)
        return false;

    object_construct(obj);

    while (CURRENT_CHAR(*parser) != '}') {
        if (parser_clean(parser))
            continue;

        if (CURRENT_CHAR(*parser) == '\0')
            return false;

        if (!parse_as_string(parser, &tmp_key, false))
            return false;

        parser_clean(parser);

        if (CURRENT_CHAR(*parser) != ':')
            return false;

        parser_advance(parser, 1);
        parser_clean(parser);

        if (!parse_as_value(parser, &tmp_val))
            return false;

        object_set(obj, tmp_key, &tmp_val);
        parser_clean(parser);

        if (CURRENT_CHAR(*parser) == ',') {
            parser_advance(parser, 1);
            parser_clean(parser);
            if (CURRENT_CHAR(*parser) == '}')
                return false;
        }
    }

    parser_advance(parser, 1);
    parser_clean(parser);
    *out = obj;
    return true;
}

static bool parse_as_null(struct JsonParser* const parser) {
    if (match(parser, "null"))
        return true;
    return false;
}

static bool parse_as_bool(struct JsonParser* const parser, bool* const out) {
    if (match(parser, "true")) {
        *out = true;
        return true;
    }
    if (match(parser, "false")) {
        *out = false;
        return true;
    }
    return false;
}

static bool parse_as_value(struct JsonParser* const parser, struct Value* const out) {
    struct Value tmp;
    parser_clean(parser);
    if (parse_as_number(parser, &out->as.number)) out->type = Number;
    else if (parse_as_string(parser, &out->as.string, true)) out->type = String;
    else if (parse_as_array(parser, &out->as.array)) out->type = Array;
    else if (parse_as_object(parser, &out->as.object)) out->type = Object;
    else if (parse_as_null(parser)) out->type = Null;
    else if (parse_as_bool(parser, &out->as.bool_)) out->type = Bool;
    else return false; /* failed to parse as anything :^( */
    parser_clean(parser);

    if (parse_as_number(parser, &tmp.as.number))       return false;
    if (parse_as_string(parser, &tmp.as.string, true)) return false;
    if (parse_as_array(parser, &tmp.as.array))         return false;
    if (parse_as_object(parser, &tmp.as.object))       return false;
    if (parse_as_null(parser))                         return false;
    if (parse_as_bool(parser, &tmp.as.bool_))          return false;
    return true;
}

struct Value *parse(char* const stream) {
    struct JsonParser parser;
    parser_construct(&parser, stream);
    if (!parse_as_value(&parser, parser.head))
        return NULL;
    return parser.head;
}

void print_number(const double number) {
    if (json_print_colored) printf("\033[34;1m");
    printf("%f", number);
    if (json_print_colored) printf("\033[0m");
}

void print_string(const char *string) {
    if (json_print_colored) printf("\033[32m");

    printf("%c", json_print_double_quoted ? '"' : '\'');
    for (; *string; string++) {
        switch (*string) {
        case '\\': printf("\\\\"); break;
        case '\b': printf("\\b"); break;
        case '\f': printf("\\f"); break;
        case '\n': printf("\\n"); break;
        case '\r': printf("\\r"); break;
        case '\t': printf("\\t"); break;
        default: printf("%c", *string);
        }
    }
    printf("%c", json_print_double_quoted ? '"' : '\'');
    if (json_print_colored) printf("\033[0m");
}

void print_null(void) {
    if (json_print_colored) printf("\033[0;1mnull\033[0m");
    else printf("null");
}

void print_bool(const bool b) {
    if (json_print_colored) printf("\033[33m");
    printf("%s", b ? "true" : "false");
    if (json_print_colored) printf("\033[0m");
}

void print_array(const struct Array* const array) {
    size_t i;
    printf("[ ");
    for (i = 0; i < array->written; i++) {
        print_value(array_at(array, i));
        if (i + 1 < array->written)
            printf(", ");
    }
    printf(" ]");
}

void print_object(const struct Object* const object) {
    struct Node *node;
    size_t printed, i;
    printed = 0;
    printf("{ ");
    for (i = 0; i < object->allocated; i++) {
        for (node = object->buckets[i]; node != NULL; node = node->next) {
            if (json_print_key_as_string)
                print_string(node->key);
            else
                printf("%s", node->key);

            printf(": ");
            print_value(&node->value);
            printed++;
            if (printed < object->pairs)
                printf(", ");
        }
    }
    printf(" }");
}

void print_value(const struct Value *const val) {
    switch (val->type) {
    case Number: print_number(val->as.number); break;
    case String: print_string(val->as.string); break;
    case Array: print_array(val->as.array); break;
    case Object: print_object(val->as.object); break;
    case Null: print_null(); break;
    case Bool: print_bool(val->as.bool_); break;
    }
}

