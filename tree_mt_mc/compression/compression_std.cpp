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

string find_head_comp_prefix(string lowerbound, string upperbound){
    string prefix = get_common_prefix(lowerbound, upperbound);
    int lowerlen = lowerbound.length();
    int upperlen = upperbound.length();
    int prefixlen = prefix.length();
    if (upperlen == prefixlen + 1 && lowerlen >= prefixlen + 1 && (upperbound.at(prefixlen) - lowerbound.at(prefixlen)) == 1){
        // cause hkey exist in next node, here works for case where node:
        // lkey = abc, hkey = abd
        // node: abcd, abce, abcf, ...
        return prefix + lowerbound.at(prefixlen);
    }
    return prefix;
}

string find_multicol_head_comp_prefix(string low_key, string high_key){
    string prefix;
    int low_idx = 0;
    int high_idx = 0;
    while(1){
        int low_pos = low_key.find('|', low_idx);
        int high_pos = high_key.find('|', high_idx);
        if (low_pos == std::string::npos){
            // last column
            string low_cur_key = low_key.substr(low_idx);
            string high_cur_key = high_key.substr(high_idx);
            prefix += find_head_comp_prefix(low_cur_key, high_cur_key);
            break;
        } else {
            string low_cur_key = low_key.substr(low_idx, low_pos - low_idx - 1);
            string high_cur_key = high_key.substr(high_idx, high_pos - high_idx - 1);
            prefix += find_head_comp_prefix(low_cur_key, high_cur_key);
            prefix += "|";
            low_idx = low_pos + 1;  
            high_idx = high_pos + 1;
        }
    }
    return prefix;
}

string multicol_cut_prefix(string_view key, string_view prefix){
    string res;
    int cursor = 0;
    int pfx_cursor = 0;
    if(prefix.length() == 0){
        res = key;
        return res;
    }
    while(1){
        int keypos = key.find('|', cursor);
        int pfxpos = prefix.find('|', pfx_cursor);
        if(keypos != std::string::npos){
            int pfx_len = pfxpos - pfx_cursor;
            // key_len - pfx_len  
            int sfx_len = (keypos - cursor) - pfx_len;
            res += key.substr(cursor + pfx_len, sfx_len);
            res += "|";
        }else{
            // last key
            int pfx_len = prefix.length() - pfx_cursor;
            res += key.substr(cursor + pfx_len);
            break;
        }
        cursor = keypos + 1;
        pfx_cursor = pfxpos + 1; 
    }
    return res;
}

string multicol_recover_prefix(string_view compkey, string_view prefix){
    string res;
    int cursor = 0;
    int pfx_cur = 0;
    while(1){
        int keypos = compkey.find('|', cursor);
        int pfxpos = prefix.find('|', pfx_cur);
        if(keypos != std::string::npos){
            res += prefix.substr(pfx_cur, pfxpos - pfx_cur); 
            res += compkey.substr(cursor, keypos - cursor);
            res += "|";
        }else{
            // last key
            res += prefix.substr(pfx_cur); 
            res += compkey.substr(cursor);
            break;
        }
        cursor = keypos + 1;
        pfx_cur = pfxpos + 1; 
    }
    if(res.length() > 32){
        cout << "[err]: recover prefix" << endl;
    }
    return res;
}

vector<string> update_prefix_on_keys(vector<string> oldkeys, string oldpfx, string newpfx){
    // the prefix can only grow !! not necessarily
    vector<string> newkeys;
    // // 1. find the difference between new prefix and old prefix
    // string pfx_diff;
    // int old_cur = 0;
    // int new_cur = 0;
    // if(oldpfx.length() == 0){
    //     pfx_diff = newpfx;
    // } else{
    //     while(1){
    //         int old_pos = oldpfx.find('|', old_cur);
    //         int new_pos = newpfx.find('|', new_cur);
    //         if (old_pos == std::string::npos){
    //             // last column
    //             int newlen = newpfx.length() - new_cur;
    //             int oldlen = oldpfx.length() - old_cur;
    //             pfx_diff += newpfx.substr(new_cur + oldlen, newlen - oldlen);
    //             break;
    //         } else{
    //             int newlen = new_pos - new_cur;
    //             int oldlen = old_pos - old_cur;
    //             pfx_diff += newpfx.substr(new_cur + oldlen, newlen - oldlen);
    //             pfx_diff += "|";
    //             old_cur = old_pos + 1;
    //             new_cur = new_pos + 1;
    //         }    
    //     }
    // }
    // // 2. subtract the difference from the keys 
    // for(auto k : oldkeys){
    //     newkeys.push_back(multicol_cut_prefix(k, pfx_diff));
    // }

    for(auto k : oldkeys){
        string s = multicol_recover_prefix(k, oldpfx);
        newkeys.push_back(multicol_cut_prefix(s, newpfx));
    }
    return newkeys;
}


