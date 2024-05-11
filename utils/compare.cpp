#pragma once
#include <algorithm>
#include <iostream>
#include <stack>
#include <string>
#include "../tree/node.h"
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
    uint16_t size = PV_SIZE;
    return char_cmp_skip(a, b - PV_SIZE, alen, blen, (uint16_t*)&size); //insane pointer arithmatic
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

#ifdef PV
long word_cmp(Stdhead* header,const char* key, int keylen) {
    // char word[8] = {0};
    // char prefix[8] = {0};
    // for (int i = 0; i < PV_SIZE; i++) 
    //     prefix[i] = header->key_prefix[PV_SIZE - 1 - i];
    // for (int i = 0; i < min(keylen, PV_SIZE); i++)
    //     word[i] = key[min(keylen, PV_SIZE) - 1 - i];
    // return *(long*)word - *(long*)prefix;
    int cmp_len = min(PV_SIZE, keylen);
    // int idx = *matchp;
    for (int idx = 0; idx < cmp_len; ++idx) {
        int cmp = key[idx] - header->key_prefix[idx];
        if (cmp != 0)
            return cmp;
    }
    /* Contents are equal up to the smallest length. */
    return 0;
}

inline long pvComp(Stdhead* header,const char* key, int keylen, Node *cursor) {
    long cmp = word_cmp(header, key, keylen);
    if (cmp == 0) {
        cmp = char_cmp_new(key, PageOffset(cursor, header->key_offset),
                            keylen, header->key_len);
    }
    return cmp;
}
#endif