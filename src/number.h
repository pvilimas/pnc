#ifndef NUMBER_H
#define NUMBER_H

// wrapper around gmp.h
#include <gmp.h>
// #include <mpfr.h>
// redefines mpf_xxx(...) to mpfr_(..., MPFR_RNDN)
// #include <mpf2mpfr.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dstr.h"

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
    union {

        // NUM_INTEGER
        mpz_t integer_value;

        // NUM_RATIONAL
        mpq_t rational_value;

        // NUM_REAL
        struct {
            mpf_t real_value;
        };
   };

} Number;

// constructors
Number number_integer_from_u32(uint32_t x);
Number number_rational_from_u32s(uint32_t p, uint32_t q); // = p/q
Number number_real_from_d(double d);
// Number number_real_from_parts(uint32_t w, double d); // w.d
// ...

// string conversion - generic versions

// never fails, the other _to_str functions also don't
char* num_to_str(Number n);

// may fail - either returns false or writes to out
// the other _from_str functions work the exact same way
bool num_from_str(char* str, int len, Number* out);

char* num_integer_to_str(Number n);
bool num_integer_from_str(char* str, int len, Number* out);

char* num_rational_to_str(Number n);
bool num_rational_from_str(char* str, int len, Number* out);

char* num_real_to_str(Number n);
bool num_real_from_str(char* str, int len, Number* out);

// conversion between gmp types
// gmp requires that these functions use out params

void Z_to_Q(mpz_t z, mpq_t q_out);
void Z_to_R(mpz_t z, mpf_t r_out);
void Q_to_R(mpq_t q, mpf_t r_out);

// here Z means integer, Q means rational, R means real
// binary operators will cast types like this:
// Z + Z = Z
// Z + Q = Q
// Z + R = R
// Q + Q = Q
// Q + R = R
// R + R = R

// arithmetic operators

Number num_add(Number n1, Number n2);
void num_add_ZZ_Z(Number n1, Number n2, Number* out);
void num_add_ZQ_Q(Number n1, Number n2, Number* out);
// void num_add_ZR_R(Number n1, Number n2, Number* out);

void num_add_QZ_Q(Number n1, Number n2, Number* out);
void num_add_QQ_Q(Number n1, Number n2, Number* out);
// void num_add_QR_R(Number n1, Number n2, Number* out);
//
// void num_add_RZ_R(Number n1, Number n2, Number* out);
// void num_add_RQ_R(Number n1, Number n2, Number* out);
// void num_add_RR_R(Number n1, Number n2, Number* out);

// comparison operators

#endif // NUMBER_H
