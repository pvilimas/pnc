#ifndef DSTR_H
#define DSTR_H

#include <string.h>
#include <stdlib.h>

// dynamic string type

typedef struct {
    char* data;
    int len;
} dstr;

#define dstr_new() \
    ((dstr){0})

// pass by value
#define dstr_append(ds, cstr) \
    do { \
        int new_len = (ds).len + strlen(cstr); \
        (ds).data = realloc((ds).data, (new_len + 1) * sizeof(char)); \
        (ds).len = new_len; \
        strcat((ds).data, cstr); \
    } while(0)

#define dstr_append_char(ds, c) \
    do { \
        char c2[2] = {c, '\0'}; \
        dstr_append(ds, c2); \
    } while(0)

#endif // DSTR_H
