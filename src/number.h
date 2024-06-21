#ifndef NUMBER_H
#define NUMBER_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// wrapper around gmp.h
#include <gmp.h>

#include <mpfr.h>
// redefines mpf_xxx(...) to mpfr_(..., MPFR_RNDN)
// #include <mpf2mpfr.h>

#include "dstr.h"

#define min(x, y) \
    (((x) < (y)) ? (x) : (y))

#define max(x, y) \
    (((x) > (y)) ? (x) : (y))


typedef enum {

    // represented with one mpz
    NUM_INTEGER,

    // represented as an exact fraction (mpq)
    NUM_RATIONAL,

    // represented with a whole part and a decimal part (mpz, mpf)
    NUM_REAL,

    // TODO add
    // NUM_COMPLEX
} NumType;

typedef struct {
    NumType type;

    uint8_t base;

    union {

        // NUM_INTEGER
        mpz_t integer_value;

        // NUM_RATIONAL
        mpq_t rational_value;

        // NUM_REAL
        mpfr_t real_value;
   };

} Number;

// ...

// output directly to stdout
void num_print(Number n);
void num_print_integer(Number n);
void num_print_rational(Number n);
void num_print_real(Number n);
void print_base_prefix(uint8_t base);

// string conversion - generic versions

// never fails, the other _to_str functions also don't
char* num_to_str(Number n);

// may fail - either returns false or writes to out
// the other _from_str functions work the exact same way
bool num_from_str(char* str, int len, Number* out);
bool num_integer_from_str(char* str, int len, Number* out,
    uint8_t base);
bool num_rational_from_str(char* str, int len, Number* out,
    uint8_t base);
bool num_real_from_str(char* str, int len, Number* out,
    uint8_t base);

// conversion between gmp types
// gmp requires that these functions use out params

void Z_to_Q(mpz_t z, mpq_t q_out);
void Z_to_R(mpz_t z, mpfr_t r_out);
void Q_to_R(mpq_t q, mpfr_t r_out);

// here Z means integer, Q means rational, R means real
// binary operators will cast types like this:
// Z + Z = Z
// Z + Q = Q
// Z + R = R
// Q + Q = Q
// Q + R = R
// R + R = R

// if two arguments to an arithmetic function have different bases,
// the first base will be used for the output
// 0b1111 + 10 = 0b11010

// arithmetic operators

Number num_add(Number n1, Number n2);
void num_add_ZZ_Z(Number n1, Number n2, Number* out, uint8_t out_base);
void num_add_ZQ_Q(Number n1, Number n2, Number* out, uint8_t out_base);
void num_add_ZR_R(Number n1, Number n2, Number* out, uint8_t out_base);

void num_add_QZ_Q(Number n1, Number n2, Number* out, uint8_t out_base);
void num_add_QQ_Q(Number n1, Number n2, Number* out, uint8_t out_base);
void num_add_QR_R(Number n1, Number n2, Number* out, uint8_t out_base);

void num_add_RZ_R(Number n1, Number n2, Number* out, uint8_t out_base);
void num_add_RQ_R(Number n1, Number n2, Number* out, uint8_t out_base);
void num_add_RR_R(Number n1, Number n2, Number* out, uint8_t out_base);

// comparison operators

#endif // NUMBER_H
