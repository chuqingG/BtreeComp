#pragma once
#include <iostream>
#include "../node.cpp"
// #include "../node_inline.h"

int compute_prefix_myisam(string prev_key, string key) {
    int pfx = 0;
    int pfx_max = min(key.length(), prev_key.length());

    for (pfx = 0; pfx < pfx_max; ++pfx) {
        if (prev_key.at(pfx) != key.at(pfx))
            break;
    }

    return pfx;
}

void get_full_key(NodeMyISAM *node, int idx, WTitem &key) {
    enum {
        FORWARD,
        BACKWARD
    } direction;

    // string prev_key = "", curr_key = "";
    direction = BACKWARD;

    MyISAMhead *head = GetHeaderMyISAM(node, idx);

    if (head->pfx_len == 0) {
        key.addr = PageOffset(node, head->key_offset);
        key.size = head->key_len;
        return;
    }

    char *prev_key;
    for (int pos = idx - 1;;) {
        MyISAMhead *head_i = GetHeaderMyISAM(node, pos);
        // int prefix = node->keys.at(pos).getPrefix();
        // string suffix = node->keys.at(pos).value;

        if (head_i->pfx_len == 0) {
            key.addr = PageOffset(node, head_i->key_offset);
            key.size = head_i->key_len;
            // curr_key = suffix;
            direction = FORWARD;
        }
        else {
            if (direction == FORWARD) {
                // curr_key = prev_key.substr(0, prefix) + suffix;
                key.size = head_i->pfx_len + head_i->key_len;
                char *newbuf = new char[key.size + 1];
                strncpy(newbuf, prev_key, head_i->pfx_len);
                strcpy(newbuf + head_i->pfx_len, PageOffset(node, head_i->key_offset));
                key.addr = newbuf;
                key.newallocated = true;
            }
        }

        if (pos == ind)
            break;

        switch (direction) {
        case BACKWARD:
            --pos;
            break;
        case FORWARD:
            ++pos;
            break;
        }

        prev_key = key.addr;
    }
    return;
}

string get_uncompressed_key_before_insert(NodeMyISAM *node, int ind, int insertpos, string newkey, bool equal) {
    if (equal) {
        return get_key(node, ind);
    }
    string uncompressed;
    if (insertpos == ind) {
        uncompressed = newkey;
    }
    else {
        int origind = insertpos < ind ? ind - 1 : ind;
        uncompressed = get_key(node, origind);
    }
    return uncompressed;
}

void build_page_prefixes(NodeMyISAM *node, int pos, string key_at_pos) {
    if (node->size == 0)
        return;
    string prev_key = key_at_pos;
    int prev_pfx = node->keys.at(pos).getPrefix();
    for (uint32_t i = pos + 1; i < node->keys.size(); i++) {
        string currkey = prev_key.substr(0, node->keys.at(i).getPrefix()) + node->keys.at(i).value;
        int curr_pfx = compute_prefix_myisam(prev_key, currkey);
        node->keys.at(i).value = currkey.substr(curr_pfx);
        node->keys.at(i).setPrefix(curr_pfx);
        prev_pfx = curr_pfx;
        prev_key = currkey;
    }
}

void update_next_prefix(NodeMyISAM *node, int pos, char *fullkey_before_pos,
                        char *full_newkey, int keylen) {
    // if newkey is the last one
    if (node->size <= 1 || pos + 1 == node->size)
        return;
    MyISAMhead *header = GetHeaderMyISAM(node, pos + 1);

    int cur_len = header->pfx_len + header->key_len;

    // build k[pos+1] from k[pos - 1]
    char *fullkey = new char[cur_len + 1];
    strncpy(fullkey, fullkey_before_pos, header->pfx_len);
    strcpy(fullkey + header->pfx_len, PageOffset(node, header->key_offset));

    // compare k[pos] and k[pos+1]
    uint8_t newpfx_len = get_common_prefix_len(full_newkey, fullkey,
                                               keylen, cur_len);
    // under many cases, newpfx is shorter than prevpfx,
    // then we need to store longer value in the buf
    if (newpfx_len > header->pfx_len) {
        // shift the pointer
        header->key_len -= newpfx_len - header->pfx_len;
        header->key_offset += newpfx_len - header->pfx_len;
        // update after use
        header->pfx_len = newpfx_len;
    }
    else if (newpfx_len < header->pfx_len) {
        // write new item
        strcpy(BufTop(node), fullkey + newpfx_len);
        header->key_offset = node->space_top;
        header->key_len = cur_len - newpfx_len;
        header->pfx_len = newpfx_len;
        node->space_top += header->key_len + 1;
    }
}