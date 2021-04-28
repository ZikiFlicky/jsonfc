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

void array_construct(struct Array *array) {
    array->allocated = 0;
    array->written = 0;
    array->arr_dump = NULL;
}

void array_dealloc(struct Array* const array) {
    free(array->arr_dump);
    free(array);
}

bool array_push(struct Array* const array, struct Value* const value) {
    if (array->allocated == 0) {
        array->allocated = ARRAY_SIZE_START;
        if ((array->arr_dump = malloc(ARRAY_SIZE_START * sizeof(struct Value))) == NULL)
            return 0;
    } else if (array->written >= array->allocated) {
        array->allocated += 8;
        if ((array->arr_dump = realloc(array->arr_dump, array->allocated * sizeof(struct Value))) == NULL)
            return 0;
    }
    array->arr_dump[array->written] = *value;
    array->written++;
    return 1;
}

struct Value *array_at(const struct Array* const array, const uint idx) {
    if (idx < array->written)
        return &array->arr_dump[idx];
    return NULL;
}


static uint object_hash(struct Object *obj, char* const to_hash) {
    uint value;
    char *cur_idx;
    value = 0;
    for (cur_idx = to_hash; *cur_idx; cur_idx++) {
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
    uint i;
    for (i = 0; i < obj->allocated; i++)
        node_delete(obj->buckets[i]);
    free(obj);
}

void object_set(struct Object *obj, char *key, struct Value *value) {
    struct Node *node;
    struct Node *previous_node;
    uint hash_result;

    if (!obj->buckets) {
        obj->buckets = malloc(sizeof(struct Node *) * OBJECT_BUCKET_AMOUNT_DEFAULT);
        obj->allocated = OBJECT_BUCKET_AMOUNT_DEFAULT;
    }

    hash_result = object_hash(obj, key);
    node = obj->buckets[hash_result];
    previous_node = NULL;

    while (node != NULL) {
        if (strcmp(key, node->key) == 0) {
            /* deallocate value if already exists at key */
            value_delete(&node->value);
            node->value = *value;
            return;
        }
        previous_node = node;
        node = node->next;
    }

    node = malloc(sizeof(struct Node));
    node->key = malloc((strlen(key) + 1) * sizeof(char));
    strcpy(node->key, key);
    node->key[strlen(key)] = 0;
    node->value = *value;

    if (previous_node == NULL)
        obj->buckets[hash_result] = node;
    else
        previous_node->next = node;
    obj->pairs++;
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
