#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "../node_mt.h"
#include "../util.cpp"

using namespace std;

#ifndef CHARALL
string tail_compress(string lastleft, string firstright){
    string prefix = get_common_prefix(lastleft, firstright);
    string compressed = prefix;
    if (firstright.length() > prefix.length())
    {
        compressed = compressed + firstright.at(prefix.length());
    }
    return compressed;
}
#else
string tail_compress(const char* lastleft, const char* firstright){
    string prefix = get_common_prefix(lastleft, firstright);
    string compressed = prefix;
    if (strlen(firstright) > prefix.length()){
        //TODO: it that correct?
        compressed = compressed + firstright[prefix.length()];
    }
    return compressed;
}
#endif

#ifndef CHARALL
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
#else
string head_compression_find_prefix(const char* lowerbound, const char* upperbound){
    string prefix = get_common_prefix(lowerbound, upperbound);
    int prefixlen = prefix.length();
    if (strlen(upperbound) == prefixlen + 1 
        && strlen(lowerbound) >= prefixlen + 1 
        && (upperbound[prefixlen] - lowerbound[prefixlen] == 1))
    {
        return prefix + lowerbound[prefixlen];
    }
    return prefix;
}
#endif

#ifndef CHARALL
void prefix_compress_vector(vector<Key> &keys, string origprefix, int prefixlen){
    vector<string> compressedkeys;
    for (uint32_t i = 0; i < keys.size(); i++)
    {
        string key = origprefix + keys.at(i).value;
        string compressedkey = key.substr(prefixlen, key.length() - prefixlen);
        keys.at(i).value = compressedkey;
    }
}
#else
void prefix_compress_vector(vector<Key_c> &keys, string origprefix, int prefixlen){
    // vector<string> res;
    // for(auto k : keys){
    //     string s(k.value);
    //     res.push_back(s);
    // }
    int len = strlen(keys[0].value);
    for (uint32_t i = 0; i < keys.size(); i++){
        string key = origprefix + keys.at(i).value;
        string compressedkey = key.substr(prefixlen, key.length() - prefixlen);
        // if(compressedkey.size() > 64)
        //     cout << "overflow" << endl;
        keys.at(i).update_value(compressedkey);
    }
    // if(len <= strlen(keys[0].value))
    //     cout << "error update" << endl;
    // for(auto k : keys){
    //     if(strlen(k.value) != 64 - prefixlen)
    //         cout << "error update" << endl;;
    // }
    //TODO: delete me
}
#endif