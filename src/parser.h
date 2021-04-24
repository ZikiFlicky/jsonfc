#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "types.h"

#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
#define bool unsigned char
#endif

typedef unsigned int uint;

#define CURRENT_CHAR(p) ((p).stream[(p).idx])
#define CHAR_AT(p, i) ((p).stream[(p).idx + (i)])


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
        struct Array *array;
        struct Object *object;
        bool bool_;
    } as;
};

struct JsonParser {
    char *stream;
    uint idx, line, column;
    struct Value *head;
};

void parser_construct(struct JsonParser *parser, char *stream);

struct Value *parse(char *stream);

void string_dealloc(char *string);

void value_delete(struct Value *value);

#endif /* JSON_PARSER_H */
