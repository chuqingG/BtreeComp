#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "../util.cpp"

using namespace std;


string tail_compress(string lastleft, string firstright){
    string prefix = get_common_prefix(lastleft, firstright);
    string compressed = prefix;
    if (firstright.length() > prefix.length())
    {
        compressed = compressed + firstright.at(prefix.length());
    }
    return compressed;
}

string head_compression_find_prefix(string lowerbound, string upperbound){
    string prefix = get_common_prefix(lowerbound, upperbound);
    int lowerlen = lowerbound.length();
    int upperlen = upperbound.length();
    int prefixlen = prefix.length();
    if (upperlen == prefixlen + 1 && lowerlen >= prefixlen + 1 && (upperbound.at(prefixlen) - lowerbound.at(prefixlen)) == 1)
    {
        return prefix + lowerbound.at(prefixlen);
    }
    return prefix;
}

#ifdef DUPKEY
void prefix_compress_vector(vector<Key_c> &keys, string origprefix, int prefixlen){
    for (uint32_t i = 0; i < keys.size(); i++)
    {
        string key = origprefix + keys.at(i).value;
        string compressedkey = key.substr(prefixlen, key.length() - prefixlen);
        keys.at(i).update_value(compressedkey);
    }
}
#else
void prefix_compress_vector(vector<char*> &keys, string origprefix, int prefixlen){
#endif
    for (uint32_t i = 0; i < keys.size(); i++)
    {
        string key = origprefix + keys.at(i);
        // The prefix after split should be longer, so strcpy is safe
        strcpy(keys.at(i), key.data() + prefixlen);
    }
}