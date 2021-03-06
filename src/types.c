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

#include "types.h"
#include "parser.h"

#include <stdlib.h>
#include <string.h>

void value_dealloc(struct Value *value) {
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

void array_construct(struct Array *array) {
    array->allocated = 0;
    array->written = 0;
    array->arr_dump = NULL;
}

void array_dealloc(struct Array* const array) {
    free(array->arr_dump);
    free(array);
}

bool array_push(struct Array* const array, struct Value value) {
    struct Value *tmp_heap;

    if (array->written >= array->allocated) {
        /* change allocation to be 8 blocks bigger and realloc to the new size */
        array->allocated += 8;
        tmp_heap = realloc(array->arr_dump, array->allocated * sizeof(struct Value));

        if (tmp_heap == NULL) {
            array->allocated -= 8;
            return false;
        }

        array->arr_dump = tmp_heap;
    }

    array->arr_dump[array->written] = value;
    ++array->written;

    return true;
}

struct Value *array_at(const struct Array* const array, const size_t idx) {
    if (idx < array->written)
        return &array->arr_dump[idx];
    return NULL;
}


static size_t object_hash(struct Object *obj, char* const to_hash) {
    size_t value = 0;
    char *cur_idx;

    for (cur_idx = to_hash; *cur_idx; ++cur_idx) {
        value *= 256;
        value += *cur_idx;
        value %= obj->allocated;
    }

    return value;
}

void object_construct(struct Object *obj) {
    obj->buckets = NULL;
    obj->allocated = 0;
    obj->pairs = 0;
}

static void node_delete(struct Node *node) {
    if (node == NULL)
        return;
    free(node->key);
    node_delete(node->next);
    free(node);
}

void object_dealloc(struct Object *obj) {
    size_t i;
    for (i = 0; i < obj->allocated; ++i)
        node_delete(obj->buckets[i]);
    free(obj);
}

bool object_set(struct Object *obj, char *key, struct Value *value) {
    struct Node *node;
    struct Node *previous_node;
    size_t hash_result;

    if (!obj->buckets) {
        obj->buckets = malloc(sizeof(struct Node *) * OBJECT_BUCKET_AMOUNT_DEFAULT);
        if (obj->buckets == NULL)
            return false;
        obj->allocated = OBJECT_BUCKET_AMOUNT_DEFAULT;
    }

    hash_result = object_hash(obj, key);
    node = obj->buckets[hash_result];
    previous_node = NULL;

    while (node != NULL) {
        if (strcmp(key, node->key) == 0) {
            /* deallocate value if already exists at key */
            value_dealloc(&node->value);
            node->value = *value;
            return true;
        }
        previous_node = node;
        node = node->next;
    }

    node = malloc(sizeof(struct Node));

    if (node == NULL)
        return false;

    node->key = malloc((strlen(key) + 1) * sizeof(char));
    if (node->key == NULL)
        return false;

    strcpy(node->key, key);
    node->key[strlen(key)] = 0;
    node->value = *value;

    if (previous_node == NULL)
        obj->buckets[hash_result] = node;
    else
        previous_node->next = node;

    ++obj->pairs;
    return true;
}

struct Value *object_get(struct Object *obj, char* key) {
    struct Node *node;

    if (obj->buckets == NULL) /* not yet allocated (object_set wasn't yet called) */
        return NULL;
    for (node = obj->buckets[object_hash(obj, key)]; node != NULL; node = node->next) {
        if (strcmp(key, node->key) == 0)
            return &node->value;
    }

    return NULL;
}

