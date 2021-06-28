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

#ifndef JSON_TYPES_H
#define JSON_TYPES_H

#include <stddef.h>

#if __STDC_VERSION__ >= 199901L
# include <stdbool.h>
#else
# define bool unsigned char
# define true 1
# define false 0
#endif

#define OBJECT_BUCKET_AMOUNT_DEFAULT 8


struct Value {
    enum ValueType {
        Number = 1,
        String,
        Array,
        Object,
        Null,
        Bool
    } type;

    union {
        double number;
        char *string;
        bool bool_;
        struct Array *array;
        struct Object *object;
    } as;
};

struct Array {
    struct Value *arr_dump;
    size_t allocated, written;
};

struct Object {
    struct Node {
        struct Node *next;
        char *key;
        struct Value value;
    } **buckets;
    size_t allocated, pairs;
};

void value_dealloc(struct Value *value);

void array_construct(struct Array *array);

void array_dealloc(struct Array* const array);

bool array_push(struct Array* const array, struct Value *value);

struct Value *array_at(const struct Array* const array, const size_t idx);

void object_construct(struct Object *obj);

bool object_set(struct Object *obj, char *key, struct Value *value);

void object_dealloc(struct Object *obj);

struct Value *object_get(struct Object *obj, char *key);

#endif /* JSON_TYPES_H */
