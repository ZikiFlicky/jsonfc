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
