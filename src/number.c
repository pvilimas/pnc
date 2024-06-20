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
    mpf_init_set_d(n.real_value, d);
    return n;
}

// Number number_real_from_parts(uint32_t w, double d) {
//     Number n = { .type = NUM_REAL };
//     mpz_init_set_ui(n.real_whole_part, w);
//     mpfr_init_set_d(n.real_decimal_part, d, MPFR_RNDN);
//     return n;
// }

char* num_to_str(Number n) {
    switch (n.type) {
        case NUM_INTEGER: return num_integer_to_str(n);
        case NUM_RATIONAL: return num_rational_to_str(n);
        case NUM_REAL: return num_real_to_str(n);
        default: return NULL;
    }
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

char* num_integer_to_str(Number n) {
    int base = 10;
    return mpz_get_str(NULL, base, n.integer_value);
}

bool num_integer_from_str(char* str, int len, Number* out) {

    // because sscanf does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = number_integer_from_u32(0);
    gmp_sscanf(str_slice, "%Zd",  n.integer_value);

    *out = n;
    return true;
}

char* num_rational_to_str(Number n) {
    int base = 10;
    return mpq_get_str(NULL, base, n.rational_value);
}

bool num_rational_from_str(char* str, int len, Number* out) {

    // because sscanf does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = number_rational_from_u32s(0, 1);
    gmp_sscanf(str_slice, "%Qd",  n.rational_value);

    *out = n;
    return true;
}

char* num_real_to_str(Number n) {
    int base = 10;

    mp_exp_t exp;

    char* c = mpf_get_str(NULL, &exp, base, 0, n.real_value);
    int len = strlen(c);

    // now add in the decimal point
    // exp is the power of 10 to multiply it by

    dstr d = dstr_new();

    if (exp > 0) {
        // exp is the number of digits to the left of the .

        // print <exp> digits, then '.', then the remaining digits

        for (int i = 0; i < exp; i++) {
            dstr_append_char(d, c[i]);
        }

        dstr_append_char(d, '.');

        for (int i = exp; i < len; i++) {
            dstr_append_char(d, c[i]);
        }

    } else if (exp < 0) {
        // exp is the number of zeros between the . and the first digit

        int num_zeroes = -1 * exp;

        dstr_append(d, "0.");

        for (int i = 0; i < num_zeroes; i++) {
            dstr_append_char(d, '0');
        }

        for (int i = 0; i < len; i++) {
            dstr_append_char(d, c[i]);
        }

    } else {
        // the number starts with 0. then all of its digits
        dstr_append(d, "0.");
        for (int i = 0; i < len; i++) {
            dstr_append_char(d, c[i]);
        }
    }

    return d.data;
}

bool num_real_from_str(char* str, int len, Number* out) {

    // because sscanf does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = number_real_from_d(0.0);
    gmp_sscanf(str_slice, "%Fd",  n.real_value);

    *out = n;
    return true;
}

void Z_to_Q(mpz_t z, mpq_t q_out) {
    mpq_init(q_out);
    mpq_set_z(q_out, z);
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
        num_add_ZQ_Q(n2, n1, &result);
    }

    else if (n1.type == NUM_RATIONAL && n2.type == NUM_RATIONAL) {
        num_add_QQ_Q(n1, n2, &result);
    } else {
        printf("oops\n");
        exit(1);
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
}

void num_add_QZ_Q(Number n1, Number n2, Number* out) {
    *out = number_rational_from_u32s(0, 1);

    mpq_t q2;
    Z_to_Q(n2.integer_value, q2);
    mpq_add(out->rational_value,
        n1.rational_value,
        q2);
}

void num_add_QQ_Q(Number n1, Number n2, Number* out) {
    *out = number_rational_from_u32s(0, 1);

    mpq_add(out->rational_value,
        n1.rational_value,
        n2.rational_value);
}
