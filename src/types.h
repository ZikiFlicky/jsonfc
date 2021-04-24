#ifndef JSON_TYPES_H
#define JSON_TYPES_H

#include "parser.h"

#define ARRAY_SIZE_START 4
#define OBJECT_BUCKET_AMOUNT_DEFAULT 8

struct Array {
    struct Value *arr_dump;
    uint allocated, written;
};

struct Object {
    struct Node {
        struct Node *next;
        char *key;
        struct Value value;
    } **buckets;
    uint allocated, pairs;
};

void array_construct(struct Array *array);

void array_dealloc(struct Array *array);

bool array_push(struct Array *array, struct Value *value);

struct Value *array_at(const struct Array *array, uint idx);

void object_construct(struct Object *obj);

void object_set(struct Object *obj, char *key, struct Value *value);

void object_dealloc(struct Object *obj);

struct Value *object_get(struct Object *obj, char *key);

#endif /* JSON_TYPES_H */
