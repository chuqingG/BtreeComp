#pragma once
#include <iostream>
#include <stack>
#include <string>
#include <algorithm>
#include "../include/config.h"
using namespace std;

int lex_compare(string a, string b){
    int len, usz, tsz;
    usz = a.length();
    tsz = b.length();
    len = min(usz, tsz);
    int ind = 0;
    for (; len > 0; --len, ++ind){
        string c1(1, a.at(ind));
        string c2(1, b.at(ind));
        int cmp = c1.compare(c2);
        if (cmp != 0){
            return cmp;
        }
    }

    /* Contents are equal up to the smallest length. */
    return ((usz == tsz) ? 0 : (usz < tsz) ? -1
                                           : 1);
}

int lex_compare_skip(string a, string b, size_t *matchp){
    string userp, treep;
    size_t len, usz, tsz;
    usz = a.length();
    tsz = b.length();
    len = min(usz, tsz) - *matchp;
    int ind = *matchp;
    for (; len > 0; --len, ++ind, ++*matchp)
    {
        string c1(1, a.at(ind));
        string c2(1, b.at(ind));
        int cmp = c1.compare(c2);
        if (cmp != 0)
        {
            return cmp;
        }
    }

    /* Contents are equal up to the smallest length. */
    return ((usz == tsz) ? 0 : (usz < tsz) ? -1
                                           : 1);
}

#ifndef CHARALL
string get_common_prefix(string x, string y){
    string prefix = "";
    int i = 0, j = 0;
    while (i < x.length() && j < y.length() && x.at(i) == y.at(j))
    {
        prefix = prefix + x.at(i);
        i++;
        j++;
    }
    return prefix;
}
#else
string get_common_prefix(const char* x, const char* y){
    string prefix = "";
    int i = 0, j = 0;
    while (i < strlen(x) && j < strlen(y) && x[i] == y[j])
    {
        prefix += x[i];
        i++;
        j++;
    }
    return prefix;
}
#endif

int char_cmp(const char* a, const char* b){
	// 1 : a > b
	const size_t min_len = (strlen(a) < strlen(b)) ? strlen(a) : strlen(b);
	int r = memcmp(a, b, min_len);
	if (r == 0) {
	  if (strlen(a) < strlen(b))
	    r = -1;
	  else if (strlen(a) > strlen(b))
	    r = +1;
	}
	return r;
}

char* string_to_char(string s){
    char* cptr = new char[s.size() + 1];
    memcpy(cptr, s.data(), s.size() + 1);
    return cptr;
}