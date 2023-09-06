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

int lex_compare(string a, string b) {
    int len, usz, tsz;
    usz = a.length();
    tsz = b.length();
    len = min(usz, tsz);
    int ind = 0;
    for (; len > 0; --len, ++ind) {
        string c1(1, a.at(ind));
        string c2(1, b.at(ind));
        int cmp = c1.compare(c2);
        if (cmp != 0) {
            return cmp;
        }
    }

    /* Contents are equal up to the smallest length. */
    return ((usz == tsz) ? 0 : (usz < tsz) ? -1 :
                                             1);
}

int lex_compare_skip(string a, string b, uint8_t *matchp) {
    size_t len, usz, tsz;
    usz = a.length();
    tsz = b.length();
    len = min(usz, tsz) - *matchp;
    int ind = *matchp;
    for (; len > 0; --len, ++ind, ++*matchp) {
        string c1(1, a.at(ind));
        string c2(1, b.at(ind));
        int cmp = c1.compare(c2);
        if (cmp != 0) {
            return cmp;
        }
    }

    /* Contents are equal up to the smallest length. */
    return ((usz == tsz) ? 0 : (usz < tsz) ? -1 :
                                             1);
}

int char_cmp_skip(const char *a, const char *b,
                  int alen, int blen, uint8_t *matchp) {
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

string get_common_prefix(string x, string y) {
    string prefix = "";
    int i = 0, j = 0;
    while (i < x.length() && j < y.length() && x.at(i) == y.at(j)) {
        prefix = prefix + x.at(i);
        i++;
        j++;
    }
    return prefix;
}

int get_common_prefix_len(const char *a, const char *b, int alen, int blen) {
    int idx = 0;
    while (idx < alen && idx < blen && a[idx] == b[idx]) {
        idx++;
    }
    return idx;
}

int char_cmp(const char *a, const char *b, int alen) {
    // 1 : a > b
    int blen = strlen(b);
    // const size_t min_len = (alen < blen) ? alen : blen;
    size_t min_len;
    int len_cmp;
    if (alen < blen) {
        min_len = alen;
    }
    else {
        min_len = blen;
    }
    int r = memcmp(a, b, min_len);
    if (r == 0) {
        return alen - blen;
    }
    return r;
}

int char_cmp_old(const char *a, const char *b) {
    // 1 : a > b
    int alen = strlen(a);
    int blen = strlen(b);
    // const size_t min_len = (alen < blen) ? alen : blen;
    size_t min_len;
    int len_cmp;
    if (alen < blen) {
        min_len = alen;
    }
    else {
        min_len = blen;
    }
    int r = memcmp(a, b, min_len);
    if (r == 0) {
        return alen - blen;
    }
    return r;
}

int char_cmp_each(const char *a, const char *b) {
    // positive : a > b
    const size_t min_len = (strlen(a) < strlen(b)) ? strlen(a) : strlen(b);
    for (int i = 0; i < min_len; i++) {
        int cmp = a[i] - b[i];
        if (cmp != 0) {
            return cmp;
        }
    }

    return strlen(a) - strlen(b);
}

char *string_to_char(string s) {
    char *cptr = new char[s.size() + 1];
    memcpy(cptr, s.data(), s.size() + 1);
    return cptr;
}