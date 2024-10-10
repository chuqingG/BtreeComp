#pragma once
#include <iostream>
#include "../tree/node.cpp"
// #include "../node_inline.h"
#include "../utils/compare.cpp"

void get_full_key(NodeMyISAM *node, int idx, Item &key) {
    enum {
        FORWARD,
        BACKWARD
    } direction;

    direction = BACKWARD;

    MyISAMhead *head = GetHeaderMyISAM(node, idx);

    if (head->pfx_len == 0) {
        key.addr = PageOffset(node, head->key_offset);
        key.size = head->key_len;
        return;
    }

    char *prev_key = NULL;
    bool newbuf = false;
    for (int pos = idx - 1;;) {
        MyISAMhead *head_i = GetHeaderMyISAM(node, pos);

        if (head_i->pfx_len == 0) {
            key.addr = PageOffset(node, head_i->key_offset);
            key.size = head_i->key_len;
            // curr_key = suffix;
            key.newallocated = false;
            direction = FORWARD;
        }
        else {
            if (direction == FORWARD) {
                key.size = head_i->pfx_len + head_i->key_len;
                char *newbuf = new char[key.size + 1];
                strncpy(newbuf, prev_key, head_i->pfx_len);
                strcpy(newbuf + head_i->pfx_len, PageOffset(node, head_i->key_offset));

                key.addr = newbuf;
                key.newallocated = true;
            }
        }

        if (newbuf)
            delete[] prev_key;
        prev_key = key.addr;
        newbuf = key.newallocated;

        if (pos == idx)
            break;

        switch (direction) {
        case BACKWARD:
            --pos;
            break;
        case FORWARD:
            ++pos;
            break;
        }

    
    }
    return;
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
    delete[] fullkey;
}