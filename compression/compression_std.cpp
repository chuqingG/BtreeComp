#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include "../utils/item.hpp"
#include "../utils/compare.cpp"
#include <cstdint>

using namespace std;
#define movNorm(src, dest) *(int*)dest = __builtin_bswap32(*(int*)src) //double declaration

char *tail_compress(const char *lastleft, const char *firstright, int len_ll, int len_fr) {
    int prefixlen = get_common_prefix_len(lastleft, firstright, len_ll, len_fr);
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    char *separator = new char[prefixlen + 1];
    strncpy(separator, firstright, prefixlen);
    separator[prefixlen] = '\0';

    return separator;
}
char *tail_compress(char *leftprefix, char *rightprefix, const char *leftsuffix, const char *rightsuffix, int len_ll, int len_fr) {
#if defined KP
    char *left = new char[len_ll + 1];
    char *right = new char[len_fr + 1];
    movNorm(leftprefix, left);
    strcpy(left + PV_SIZE, leftsuffix);
    movNorm(rightprefix, right);
    strcpy(right + PV_SIZE, rightsuffix);
    char* ret = tail_compress(left, right, len_ll, len_fr);
    delete left;
    delete right;
    return ret;
#elif defined FN
    int prefixlength = normed_common_prefix(*(int*)leftprefix, *(int*)rightprefix);
    if (prefixlength == PV_SIZE) {
        int length = min(len_ll - PV_SIZE, len_fr - PV_SIZE);
        for (int idx = 0; idx < length; idx += 4, leftsuffix += 4, rightsuffix += 4) {
            int temp = normed_common_prefix(*(int*)leftsuffix, *(int*)rightsuffix);
            prefixlength += temp;
            if (temp < PV_SIZE) break;
        }
        if (len_fr > prefixlen) {
            prefixlen++;
        }
    }
    char *separator = new char[prefixlen + 1];
    
    strncpy(separator, firstright, prefixlen);
    separator[prefixlen] = '\0';

    return separator;
#endif
}

int tail_compress_length(const char *lastleft, const char *firstright, int len_ll, int len_fr) {
    int prefixlen = get_common_prefix_len(lastleft, firstright, len_ll, len_fr);
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    return prefixlen;
}

int tail_compress_length(char *leftprefix, char *rightprefix, const char *leftsuffix, const char *rightsuffix, int len_ll, int len_fr) {
#if defined KP
    char *left = new char[len_ll + 1];
    char *right = new char[len_fr + 1];
    movNorm(leftprefix, left);
    strcpy(left + PV_SIZE, leftsuffix);
    movNorm(rightprefix, right);
    strcpy(right + PV_SIZE, rightsuffix);
    int ret = tail_compress_length(left, right, len_ll, len_fr); //used in finding the length of tail comp key
    delete left;
    delete right;
    return ret;
#elif defined FN
    int prefixlength  = normed_common_prefix(*(int*)leftprefix, *(int*)rightprefix);
    if (prefixlength < PV_SIZE) goto finish;
    int length = min(len_ll - PV_SIZE, len_fr - PV_SIZE);
    for (int idx = 0; idx < length; idx += 4, leftsuffix += 4, rightsuffix += 4) {
        int temp = normed_common_prefix(*(int*)leftsuffix, *(int*)rightsuffix);
        prefixlength += temp;
        if (temp < PV_SIZE) break;
    }
    finish:
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    return prefixlen;
#endif
}

int head_compression_find_prefix_length(Item *low, Item *high) {
    int prefixlen = get_common_prefix_len(low->addr, high->addr, low->size, high->size);
    if (high->size == prefixlen + 1 && low->size >= prefixlen + 1 && ((high->addr)[prefixlen] - (low->addr)[prefixlen] == 1)) {
        prefixlen++;
    }
    return prefixlen;
}
