#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include "../utils/item.hpp"
#include "../utils/compare.cpp"
#include "tree/node.h"
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
char *tail_compress(Stdhead *lastleft, Stdhead *firstright, const char *leftsuffix, const char *rightsuffix) {
    char *left = new char[lastleft->key_len + 1];
    char *right = new char[firstright->key_len + 1];
    strncpy(left, lastleft->key_prefix, PV_SIZE);
    strcpy(left + PV_SIZE, leftsuffix);
    strncpy(right, firstright->key_prefix, PV_SIZE);
    strcpy(right + PV_SIZE, rightsuffix);
    return tail_compress(left, right, lastleft->key_len, firstright->key_len);
}

int tail_compress_length(const char *lastleft, const char *firstright, int len_ll, int len_fr) {
    int prefixlen = get_common_prefix_len(lastleft, firstright, len_ll, len_fr);
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    return prefixlen;
}

int tail_compress_length(Stdhead *lastleft, Stdhead *firstright, const char *leftsuffix, const char *rightsuffix) {
    char *left = new char[lastleft->key_len + 1];
    char *right = new char[firstright->key_len + 1];
    strncpy(left, lastleft->key_prefix, PV_SIZE);
    strcpy(left + PV_SIZE, leftsuffix);
    strncpy(right, firstright->key_prefix, PV_SIZE);
    strcpy(right + PV_SIZE, rightsuffix);
    return tail_compress_length(left, right, lastleft->key_len, firstright->key_len);
}

int head_compression_find_prefix_length(Item *low, Item *high) {
    int prefixlen = get_common_prefix_len(low->addr, high->addr, low->size, high->size);
    if (high->size == prefixlen + 1 && low->size >= prefixlen + 1 && ((high->addr)[prefixlen] - (low->addr)[prefixlen] == 1)) {
        prefixlen++;
    }
    return prefixlen;
}
