#include "number.h"

Number number_integer_from_u32(uint32_t x) {
    Number n = { .type = NUM_INTEGER };
    mpz_init(n.integer_value);
    mpz_set_ui(n.integer_value, x);
    return n;
}

Number number_rational_from_u32s(uint32_t p, uint32_t q) {
    Number n = { .type = NUM_RATIONAL };
    mpq_init(n.rational_value);

    mpz_t num;
    mpz_t denom;

    mpz_init_set_ui(num, p);
    mpz_init_set_ui(denom, q);

    mpq_set_num(n.rational_value, num);
    mpq_set_den(n.rational_value, denom);

    return n;
}

Number number_real_from_d(double d) {
    Number n = { .type = NUM_REAL };
    mpfr_init_set_d(n.real_value, d, MPFR_RNDN);
    return n;
}

void num_print(Number n) {
    print_base_prefix(n.base);
    switch (n.type) {
        case NUM_INTEGER: num_print_integer(n); break;
        case NUM_RATIONAL: num_print_rational(n); break;
        case NUM_REAL: num_print_real(n); break;
        default: break;
    }
}

void num_print_integer(Number n) {
    mpz_out_str(stdout, n.base, n.integer_value);
}

void num_print_rational(Number n) {
    mpq_out_str(stdout, n.base, n.rational_value);
}

void num_print_real(Number n) {
    // mpfr_printf("%Rg", n.real_value);
    mpfr_out_str(stdout, n.base, 0, n.real_value, MPFR_RNDN);
}

void print_base_prefix(uint8_t base) {
    if (base == 2) fputs("0b", stdout);
    else if (base == 8) fputs("0o", stdout);
    else if (base == 16) fputs("0x", stdout);
}

bool num_from_str(char* str, int len, Number* out) {

    if (len == 0) {
        return false;
    }

    int base = 10;
    // current valid bases:
    // 0b... => 2
    // 0o... => 8
    // 0x... => 16

    if (len >= 2 && str[0] == '0') {
        if (str[1] == 'b') base = 2;
        else if (str[1] == 'o') base = 8;
        else if (str[1] == 'x') base = 16;
    }

    // chop off base prefix before continuing to parse
    if (base != 10) {
        str += 2;
        len -= 2;
    }

    if (len == 0) {
        return false;
    }

    int dot_count = 0; // '.'
    int slash_count = 0; // '/'
    // TODO count occurrences of :
    // int e_count = 0; // 'e' or 'E'
    // int minus_count = 0; // '-'

    for (int i = 0; i < len; i++) {
        if (str[i] == '/') {
            slash_count += 1;
        } else if (str[i] == '.') {
            dot_count += 1;
        }
    }

    // something like 5.3/2 or -6/9.8
    if (slash_count != 0 && dot_count != 0) {
        return false;
    }

    if (dot_count == 1) {
        return num_real_from_str(str, len, out, base);
    } else if (dot_count > 1) { // "2.3.5"
        return false;
    }

    if (slash_count == 1) {
        return num_rational_from_str(str, len, out, base);
    } else if (slash_count > 1) { // "2//6"
        return false;
    }

    return num_integer_from_str(str, len, out, base);
}

bool num_integer_from_str(char* str, int len, Number* out, uint8_t base) {

    // because set_str does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = { .type = NUM_INTEGER, .base = base };
    int retval = mpz_set_str(n.integer_value, str_slice, base);
    if (retval == -1) {
        return false;
    }

    *out = n;
    return true;
}

bool num_rational_from_str(char* str, int len, Number* out, uint8_t base) {

    // because set_str does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = { .type = NUM_RATIONAL, .base = base };
    int retval = mpq_set_str(n.rational_value, str_slice, base);
    if (retval == -1) {
        return false;
    }

    *out = n;
    return true;
}

bool num_real_from_str(char* str, int len, Number* out, uint8_t base) {

    // because set_str does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = { .type = NUM_REAL, .base = base };
    int retval = mpfr_set_str(n.real_value, str_slice, base, MPFR_RNDN);
    if (retval == -1) {
        return false;
    }

    *out = n;
    return true;
}

void Z_to_Q(mpz_t z, mpq_t q_out) {
    mpq_init(q_out);
    mpq_set_z(q_out, z);
    mpq_canonicalize(q_out);
}

void Z_to_R(mpz_t z, mpfr_t r_out) {
    mpfr_init(r_out);
    mpfr_set_z(r_out, z, MPFR_RNDN);
}

void Q_to_R(mpq_t q, mpfr_t r_out) {
    mpfr_init(r_out);
    mpfr_set_q(r_out, q, MPFR_RNDN);
}

Number num_add(Number n1, Number n2) {

    uint8_t base = n1.base;

    Number result;

    if (n1.type == NUM_INTEGER && n2.type == NUM_INTEGER) {
        num_add_ZZ_Z(n1, n2, &result, base);
    }

    else if (n1.type == NUM_INTEGER && n2.type == NUM_RATIONAL) {
        num_add_ZQ_Q(n1, n2, &result, base);
    }

    else if (n1.type == NUM_INTEGER && n2.type == NUM_REAL) {
        num_add_ZR_R(n1, n2, &result, base);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_INTEGER) {
        num_add_QZ_Q(n1, n2, &result, base);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_RATIONAL) {
        num_add_QQ_Q(n1, n2, &result, base);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_REAL) {
        num_add_QR_R(n1, n2, &result, base);
    }

    else if (n1.type == NUM_REAL && n2.type == NUM_INTEGER) {
        num_add_RZ_R(n1, n2, &result, base);
    }

    else if (n1.type == NUM_REAL && n2.type == NUM_RATIONAL) {
        num_add_RQ_R(n1, n2, &result, base);
    }

    else if (n1.type == NUM_REAL && n2.type == NUM_REAL) {
        num_add_RR_R(n1, n2, &result, base);
    }

    return result;
}

void num_add_ZZ_Z(Number n1, Number n2, Number* out, uint8_t out_base) {
    *out = (Number){ .type = NUM_INTEGER, .base = out_base };

    mpz_add(out->integer_value,
        n1.integer_value,
        n2.integer_value);
}

void num_add_ZQ_Q(Number n1, Number n2, Number* out, uint8_t out_base) {
    mpq_t q1;
    Z_to_Q(n1.integer_value, q1);

    *out = (Number){ .type = NUM_RATIONAL, .base = out_base };

    mpq_add(out->rational_value,
        q1,
        n2.rational_value);

    mpq_canonicalize(out->rational_value);
}

void num_add_ZR_R(Number n1, Number n2, Number* out, uint8_t out_base) {
    mpfr_t r1;
    Z_to_R(n1.integer_value, r1);

    *out = (Number){ .type = NUM_REAL, .base = out_base };

    mpfr_add(out->real_value,
        r1,
        n2.real_value,
        MPFR_RNDN);
}

void num_add_QZ_Q(Number n1, Number n2, Number* out, uint8_t out_base) {
    mpq_t q2;
    Z_to_Q(n2.integer_value, q2);

    *out = (Number){ .type = NUM_RATIONAL, .base = out_base };

    mpq_add(out->rational_value,
        n1.rational_value,
        q2);

    mpq_canonicalize(out->rational_value);
}

void num_add_QQ_Q(Number n1, Number n2, Number* out, uint8_t out_base) {

    *out = (Number){ .type = NUM_RATIONAL, .base = out_base };

    mpq_add(out->rational_value,
        n1.rational_value,
        n2.rational_value);

    mpq_canonicalize(out->rational_value);
}

void num_add_QR_R(Number n1, Number n2, Number* out, uint8_t out_base) {
    mpfr_t r1;
    Q_to_R(n1.rational_value, r1);

    *out = (Number){ .type = NUM_REAL, .base = out_base };

    mpfr_add(out->real_value,
        r1,
        n2.real_value,
        MPFR_RNDN);
}

void num_add_RZ_R(Number n1, Number n2, Number* out, uint8_t out_base) {
    mpfr_t r2;
    Z_to_R(n2.integer_value, r2);

    *out = (Number){ .type = NUM_REAL, .base = out_base };

    mpfr_add(out->real_value,
        n1.real_value,
        r2,
        MPFR_RNDN);
}

void num_add_RQ_R(Number n1, Number n2, Number* out, uint8_t out_base) {
    mpfr_t r2;
    Q_to_R(n2.rational_value, r2);

    *out = (Number){ .type = NUM_REAL, .base = out_base };

    mpfr_add(out->real_value,
        n1.real_value,
        r2,
        MPFR_RNDN);
}

void num_add_RR_R(Number n1, Number n2, Number* out, uint8_t out_base) {
    *out = (Number){ .type = NUM_REAL, .base = out_base };

    mpfr_add(out->real_value,
        n1.real_value,
        n2.real_value,
        MPFR_RNDN);
}
