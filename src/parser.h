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

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "types.h"

#include <stddef.h>

#define CURRENT_CHAR(p) ((p).stream[(p).idx])
#define CHAR_AT(p, i) ((p).stream[(p).idx + (i)])

/* TODO: order this as a struct */
extern bool json_print_colored;
extern bool json_print_double_quoted;
extern bool json_print_key_as_string;

struct JsonParser {
    char *stream;
    size_t idx, line, column;
    struct Value *head;
};

void parser_construct(struct JsonParser* const parser, char* const stream);

struct Value *parse(char* const stream);

struct Value *parse_file(char* const filename);

/* printing functions */
void print_number(const double number);

void print_string(const char *string);

void print_null(void);

void print_bool(const bool b);

void print_array(const struct Array *array);

void print_object(const struct Object *object);

void print_value(const struct Value *val);

#endif /* JSON_PARSER_H */
