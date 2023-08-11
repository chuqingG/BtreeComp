#pragma once
#include <iostream>
#include "../node_mt.cpp"

int compute_prefix_myisam(string prev_key, string key){
    int pfx = 0;
    int pfx_max = min(key.length(), prev_key.length());

    for (pfx = 0; pfx < pfx_max; ++pfx){
        if (prev_key.at(pfx) != key.at(pfx))
            break;
    }

    return pfx;
}

string get_key(NodeMyISAM *node, int ind){
    enum{
        FORWARD,
        BACKWARD
    } direction;

    string prev_key = "", curr_key = "";
    direction = BACKWARD;

    if (node->keys.at(ind).getPrefix() == 0){
        return node->keys.at(ind).value;
    }

    for (int pos = ind - 1;;){
        int prefix = node->keys.at(pos).getPrefix();
        string suffix = node->keys.at(pos).value;

        if (prefix == 0){
            curr_key = suffix;
            direction = FORWARD;
        } else{
            if (direction == FORWARD){
                curr_key = prev_key.substr(0, prefix) + suffix;
            }
        }

        if (pos == ind)
            break;
        
        switch (direction){
            case BACKWARD:
                --pos;
                break;
            case FORWARD:
                ++pos;
                break;
        }

        prev_key = curr_key;
    }
    return curr_key;
}

string get_uncompressed_key_before_insert(NodeMyISAM *node, int ind, int insertpos, string newkey, bool &equal){
     if (equal) {
        return get_key(node, ind);
    }
    string uncompressed;
    if (insertpos == ind){
        uncompressed = newkey;
    } else{
        int origind = insertpos < ind ? ind - 1 : ind;
        uncompressed = get_key(node, origind);
    }
    return uncompressed;
}

void build_page_prefixes(NodeMyISAM *node, int pos, string key_at_pos)
{
    if (node->size == 0)
        return;
    string prev_key = key_at_pos;
    int prev_pfx = node->keys.at(pos).getPrefix();
    for (uint32_t i = pos + 1; i < node->keys.size(); i++)
    {
        string currkey = prev_key.substr(0, node->keys.at(i).getPrefix()) + node->keys.at(i).value;
        int curr_pfx = compute_prefix_myisam(prev_key, currkey);
        node->keys.at(i).value = currkey.substr(curr_pfx);
        node->keys.at(i).setPrefix(curr_pfx);
        prev_pfx = curr_pfx;
        prev_key = currkey;
    }
}