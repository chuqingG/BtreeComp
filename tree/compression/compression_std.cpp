#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "../node.h"
#include "../util.cpp"

using namespace std;

string tail_compress_old(string lastleft, string firstright) {
    string prefix = get_common_prefix(lastleft, firstright);
    string compressed = prefix;
    if (firstright.length() > prefix.length()) {
        compressed = compressed + firstright.at(prefix.length());
    }
    return compressed;
}

char* tail_compress(const char* lastleft, const char* firstright, int len_ll, int len_fr) {
    int prefixlen = get_common_prefix_len(lastleft, firstright, len_ll, len_fr);
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    char *separator = new char[prefixlen + 1];
    strncpy(separator, firstright, prefixlen);
    separator[prefixlen] = '\0';

    return separator;
}

int tail_compress_length(const char* lastleft, const char* firstright, int len_ll, int len_fr) {
    int prefixlen = get_common_prefix_len(lastleft, firstright, len_ll, len_fr);
    if (len_fr > prefixlen) {
        prefixlen++;
    }
    return prefixlen;
}

string head_compression_find_prefix_old(string lowerbound, string upperbound) {
    string prefix = get_common_prefix(lowerbound, upperbound);
    int lowerlen = lowerbound.length();
    int upperlen = upperbound.length();
    int prefixlen = prefix.length();
    if (upperlen == prefixlen + 1 && lowerlen >= prefixlen + 1 && (upperbound.at(prefixlen) - lowerbound.at(prefixlen)) == 1) {
        return prefix + lowerbound.at(prefixlen);
    }
    return prefix;
}

char *head_compression_find_prefix(Data *low, Data *high) {
    // string prefix = get_common_prefix(lowerbound, upperbound);
    // Can return a pair of (char*, len)
    // int lowerlen = strlen(lowerbound);
    // int upperlen = strlen(upperbound);
    int prefixlen = get_common_prefix_len(low->addr(), high->addr(), low->size, high->size);
    if (high->size == prefixlen + 1 && low->size >= prefixlen + 1 && ((high->addr())[prefixlen] - (low->addr())[prefixlen] == 1)) {
        prefixlen++;
    }

    char *prefix = new char[prefixlen + 1];
    strncpy(prefix, low->addr(), prefixlen);
    prefix[prefixlen] = '\0';

    return prefix;
}

int head_compression_find_prefix_length(Data *low, Data *high) {
    int prefixlen = get_common_prefix_len(low->addr(), high->addr(), low->size, high->size);
    if (high->size == prefixlen + 1 && low->size >= prefixlen + 1 && ((high->addr())[prefixlen] - (low->addr())[prefixlen] == 1)) {
        prefixlen++;
    }
    return prefixlen;
}

#ifdef DUPKEY
void prefix_compress_vector(vector<Key_c> &keys, string origprefix, int prefixlen) {
    for (uint32_t i = 0; i < keys.size(); i++) {
        string key = origprefix + keys.at(i).value;
        string compressedkey = key.substr(prefixlen, key.length() - prefixlen);
        keys.at(i).update_value(compressedkey);
    }
}
#else
void prefix_compress_vector(vector<char *> &keys, string origprefix, int prefixlen) {
#endif
for (uint32_t i = 0; i < keys.size(); i++) {
    string key = origprefix + keys.at(i);
    // The prefix after split should be longer, so strcpy is safe
    strcpy(keys.at(i), key.data() + prefixlen);
}
}