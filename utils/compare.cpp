#pragma once
#include <algorithm>
#include <iostream>
#include <stack>
#include <string>
using namespace std;

int get_common_prefix_len(const char *a, const char *b, int alen, int blen) {
    int idx = 0;
    while (idx < alen && idx < blen && a[idx] == b[idx]) {
        idx++;
    }
    return idx;
}

int char_cmp_skip(const char *a, const char *b,
                  int alen, int blen, uint16_t *matchp) {
    // 1 : a > b
    // const size_t min_len = (alen < blen) ? alen : blen;
    int cmp_len = min(alen, blen) - *matchp;
    int idx = *matchp;
    for (; cmp_len > 0; --cmp_len, ++idx, ++*matchp) {
        int cmp = a[idx] - b[idx];
        if (cmp != 0)
            return cmp;
    }
    /* Contents are equal up to the smallest length. */
    return (alen - blen);
}

int char_cmp_new(const char *a, const char *b, int alen, int blen) { // gauruntee to be longer than 4 in PV. Handles is 4 as well.
    // 1 : a > b
    // const size_t min_len = (alen < blen) ? alen : blen;
    #ifdef PV
    return char_cmp_skip(a, b, alen, blen, PV_SIZE);
    #else
    int cmp_len = min(alen, blen);
    // int idx = *matchp;
    for (int idx = 0; idx < cmp_len; ++idx) {
        int cmp = a[idx] - b[idx];
        if (cmp != 0)
            return cmp;
    }
    /* Contents are equal up to the smallest length. */
    return (alen - blen);
    #endif
}
