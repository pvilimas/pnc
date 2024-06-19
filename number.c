#include "number.h"

Number num_from_u32(uint32_t i) {
    Number n = { .type = NUM_INT };

    mpz_init(n.int_value);
    mpz_add_ui(n.int_value, n.int_value, i);

    return n;
};

// base is always 10 for now
bool num_from_str(char* str, int len, Number* out) {
    int base = 10;

    // because sscanf does not take a len argument, use this
    char* str_slice = strndup(str, len);

    Number n = num_from_u32(0);
    gmp_sscanf(str_slice, "%Zd",  n.int_value);

    *out = n;
    return true;
}

char* num_to_str(Number n) {
    int base = 10;

    return mpz_get_str(NULL, base, n.int_value);
}

// return n1 + n2
Number num_add(Number n1, Number n2) {

    Number result = num_from_u32(0);
    mpz_add(result.int_value, n1.int_value, n2.int_value);

    return result;
}
