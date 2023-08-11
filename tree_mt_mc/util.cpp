#pragma once
#include <iostream>
#include <stack>
#include <string>
#include <algorithm>

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

string get_common_prefix(string x, string y){
    string prefix = "";
    int i = 0, j = 0;
    while (i < x.length() && j < y.length() && x.at(i) == y.at(j)){
        prefix = prefix + x.at(i);
        i++;
        j++;
    }
    return prefix;
}