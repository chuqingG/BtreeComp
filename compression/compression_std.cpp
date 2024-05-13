#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include "../utils/item.hpp"
#include "../utils/compare.cpp"
#include "../tree/node_inline.h"

using namespace std;

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
    char *left = new char[len_ll + 1];
    char *right = new char[len_fr + 1];
    strncpy(left, leftprefix, PV_SIZE);
    strcpy(left + PV_SIZE, leftsuffix);
    strncpy(right, rightprefix, PV_SIZE);
    strcpy(right + PV_SIZE, rightsuffix);
    return tail_compress(left, right, len_ll, len_fr);
}

int tail_compress_length(const char *lastleft, const char *firstright, int len_ll, int len_fr) {
    int prefixlen = get_common_prefix_len(lastleft, firstright, len_ll, len_fr);
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    return prefixlen;
}
#ifdef STDGUARD
int tail_compress_length(char *leftprefix, char *rightprefix, const char *leftsuffix, \
                        const char *rightsuffix, int len_ll, int len_fr) { //overloading def
#ifdef KN
    word_conv_store(leftprefix, leftprefix);
    word_conv_store(rightprefix, rightprefix);
    leftsuffix = string_conv(leftsuffix, len_ll);
    rightsuffix = string_conv(rightsuffix, len_fr);
#endif
    char *left = new char[len_ll + 1];
    char *right = new char[len_fr + 1];
    strncpy(left, leftprefix, PV_SIZE);
    strcpy(left + PV_SIZE, leftsuffix);
    strncpy(right, rightprefix, PV_SIZE);
    strcpy(right + PV_SIZE, rightsuffix);
    return tail_compress_length(left, right, len_ll, len_fr);
}
#endif
int head_compression_find_prefix_length(Item *low, Item *high) {
    int prefixlen = get_common_prefix_len(low->addr, high->addr, low->size, high->size);
    if (high->size == prefixlen + 1 && low->size >= prefixlen + 1 && ((high->addr)[prefixlen] - (low->addr)[prefixlen] == 1)) {
        prefixlen++;
    }
    return prefixlen;
}
