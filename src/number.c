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
    switch (n.type) {
        case NUM_INTEGER: num_print_integer(n); break;
        case NUM_RATIONAL: num_print_rational(n); break;
        case NUM_REAL: num_print_real(n); break;
        default: break;
    }
}

void num_print_integer(Number n) {
    mpz_out_str(stdout, 10, n.integer_value);
}

void num_print_rational(Number n) {
    mpq_out_str(stdout, 10, n.rational_value);
}

void num_print_real(Number n) {
    mpfr_printf("%Rg", n.real_value);
}

bool num_from_str(char* str, int len, Number* out) {

    // analyze the string first
    // TODO count occurrences of :

    int dot_count = 0; // '.'
    int slash_count = 0; // '/'
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
        return num_real_from_str(str, len, out);
    } else if (dot_count > 1) { // 2.3.5
        return false;
    }

    if (slash_count == 1) {
        return num_rational_from_str(str, len, out);
    } else if (slash_count > 1) { // 2//6
        return false;
    }

    return num_integer_from_str(str, len, out);
}

bool num_integer_from_str(char* str, int len, Number* out) {

    // because sscanf does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = number_integer_from_u32(0);
    gmp_sscanf(str_slice, "%Zd",  n.integer_value);

    *out = n;
    return true;
}

bool num_rational_from_str(char* str, int len, Number* out) {

    // because sscanf does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = number_rational_from_u32s(0, 1);
    gmp_sscanf(str_slice, "%Qd",  n.rational_value);

    *out = n;
    return true;
}

bool num_real_from_str(char* str, int len, Number* out) {

    // because set_str does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = number_real_from_d(0.0);
    int retval = mpfr_set_str(n.real_value, str_slice, 0, MPFR_RNDN);
    if (retval == -1) {
        return false;
    }

    // to round properly, count digits after the decimal point

    int n_digits_after_dp = -1;
    for (int i = 0; i < len; i++) {
        if (n_digits_after_dp > -1)
            n_digits_after_dp += 1;

        if (str[i] == '.')
            n_digits_after_dp = 0;
    }

    *out = n;
    return true;
}

void Z_to_Q(mpz_t z, mpq_t q_out) {
    mpq_init(q_out);
    mpq_set_z(q_out, z);
    mpq_canonicalize(q_out);
}

void Q_to_R(mpq_t q, mpfr_t r_out) {
    mpfr_init(r_out);
    mpfr_set_q(r_out, q, MPFR_RNDN);
}

Number num_add(Number n1, Number n2) {

    Number result;

    if (n1.type == NUM_INTEGER && n2.type == NUM_INTEGER) {
        num_add_ZZ_Z(n1, n2, &result);
    }

    else if (n1.type == NUM_INTEGER && n2.type == NUM_RATIONAL) {
        num_add_ZQ_Q(n1, n2, &result);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_INTEGER) {
        num_add_QZ_Q(n1, n2, &result);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_RATIONAL) {
        num_add_QQ_Q(n1, n2, &result);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_REAL) {
        num_add_QR_R(n1, n2, &result);
    }

    else if (n1.type == NUM_REAL && n2.type == NUM_REAL) {
        num_add_RR_R(n1, n2, &result);
    }

    return result;
}

void num_add_ZZ_Z(Number n1, Number n2, Number* out) {
    *out = number_integer_from_u32(0);

    mpz_add(out->integer_value,
        n1.integer_value,
        n2.integer_value);
}

void num_add_ZQ_Q(Number n1, Number n2, Number* out) {
    *out = number_rational_from_u32s(0, 1);

    mpq_t q1;
    Z_to_Q(n1.integer_value, q1);
    mpq_add(out->rational_value,
        q1,
        n2.rational_value);

    mpq_canonicalize(out->rational_value);
}

void num_add_QZ_Q(Number n1, Number n2, Number* out) {
    *out = number_rational_from_u32s(0, 1);

    mpq_t q2;
    Z_to_Q(n2.integer_value, q2);
    mpq_add(out->rational_value,
        n1.rational_value,
        q2);

    mpq_canonicalize(out->rational_value);
}

void num_add_QQ_Q(Number n1, Number n2, Number* out) {
    *out = number_rational_from_u32s(0, 1);

    mpq_add(out->rational_value,
        n1.rational_value,
        n2.rational_value);

    mpq_canonicalize(out->rational_value);
}

void num_add_QR_R(Number n1, Number n2, Number* out) {
    *out = number_real_from_d(0.0);

    mpfr_t r1;
    Q_to_R(n1.rational_value, r1);
    mpfr_add(out->real_value,
        r1,
        n2.real_value,
        MPFR_RNDN);
}

void num_add_RR_R(Number n1, Number n2, Number* out) {
    *out = number_real_from_d(0.0);

    mpfr_add(out->real_value,
        n1.real_value,
        n2.real_value, MPFR_RNDN);
}
