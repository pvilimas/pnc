#ifndef NUMBER_H
#define NUMBER_H

// wrapper around gmp.h
#include <gmp.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NUM_INT
} NumType;

typedef struct {
    NumType type;
    mpz_t int_value;
} Number;

Number num_from_u32(uint32_t i);

// may fail - either returns false or writes to out
bool num_from_str(char* str, int len, Number* out);

// never fails
char* num_to_str(Number n);

Number num_add(Number n1, Number n2);

#endif // NUMBER_H
