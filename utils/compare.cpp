#pragma once
#include <algorithm>
#include <iostream>
#include <stack>
#include <string>
// #include <chrono>
using namespace std;

#define TIMECOUNT(t, f, ...)                                                  \
    {                                                                         \
        auto t1 = std::chrono::system_clock::now();                           \
        f(__VA_ARGS__);                                                       \
        auto t2 = std::chrono::system_clock::now();                           \
        double t_gap =                                                        \
            static_cast<double>(                                              \
                std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1) \
                    .count())                                                 \
            / 1e9;                                                            \
        t += t_gap;                                                           \
    }

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

int char_cmp_new(const char *a, const char *b, int alen, int blen) {
    // 1 : a > b
    // const size_t min_len = (alen < blen) ? alen : blen;
    int cmp_len = min(alen, blen);
    // int idx = *matchp;
    for (int idx = 0; idx < cmp_len; ++idx) {
        int cmp = a[idx] - b[idx];
        if (cmp != 0)
            return cmp;
    }
    /* Contents are equal up to the smallest length. */
    return (alen - blen);
}
