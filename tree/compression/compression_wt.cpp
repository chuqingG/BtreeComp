#pragma once
#include "compression_wt.h"

uint8_t compute_prefix_wt(string prev_key, string key) {
    uint8_t pfx_max;
    uint8_t pfx = 0;

    pfx_max = UINT8_MAX;
    if (key.length() < pfx_max)
        pfx_max = key.length();
    if (prev_key.length() < pfx_max)
        pfx_max = prev_key.length();

    for (pfx = 0; pfx < pfx_max; ++pfx) {
        if (prev_key.at(pfx) != key.at(pfx))
            break;
    }

    return pfx;
}

uint8_t compute_prefix_wt(string prev_key, string key, uint8_t prev_pfx) {
    uint8_t pfx_max;
    uint8_t pfx = 0;
    const char *a, *b;

    pfx_max = UINT8_MAX;
    if (key.length() < pfx_max)
        pfx_max = key.length();
    if (prev_key.length() < pfx_max)
        pfx_max = prev_key.length();

    for (pfx = 0; pfx < pfx_max; ++pfx) {
        if (prev_key.at(pfx) != key.at(pfx))
            break;
    }

    if (pfx < prefix_compression_min)
        pfx = 0;
    else if (prev_pfx != 0 && pfx > prev_pfx && pfx < prev_pfx + WT_KEY_PREFIX_PREVIOUS_MINIMUM)
        pfx = prev_pfx;

    return pfx;
}

uint8_t compute_new_prefix_len(const char *pkey, const char *ckey,
                               int pkey_len, int ckey_len, uint8_t ppfx_len) {
    int newpfx_len = get_common_prefix_len(pkey, ckey, pkey_len, ckey_len);
    if (newpfx_len < MIN_PREFIX_LEN)
        newpfx_len = 0;
    else if (ppfx_len != 0 && newpfx_len > ppfx_len
             && newpfx_len < ppfx_len + MIN_PREFIX_GAP_TO_UPDATE)
        newpfx_len = ppfx_len;
    return newpfx_len;
}

#ifdef WTCACHE
string initialize_prefix_compressed_key(NodeWT *node, int ind) {
    enum {
        FORWARD,
        BACKWARD
    } direction;
    u_int jump_slot_offset = 0;
    uint8_t last_prefix = 0;
    uint8_t key_prefix = 0;
    string key_data, group_key, initialized_value;
    bool isinitialized;

    direction = BACKWARD;
    u_int slot_offset;
    u_int rip = ind;
    u_int jump_rip = 0;

    string keyb;

    for (slot_offset = 0;;) {
        if (0) {
        switch_and_jump:
            /* Switching to a forward roll. */
            direction = FORWARD;

            /* Skip list of keys with compatible prefixes. */
            rip = jump_rip;
            slot_offset = jump_slot_offset;
        }

    overflow_retry:
        key_prefix = node->keys.at(rip).prefix;
        key_data = node->keys.at(rip).value;
        isinitialized = node->keys.at(rip).isinitialized;
        initialized_value = node->keys.at(rip).initialized_value;

        if (!isinitialized) {
            if (key_prefix == 0) {
                keyb = key_data;
            }
            else
                goto prefix_continue;

            if (slot_offset == 0) {
                return (keyb);
            }
            goto switch_and_jump;
        }

        if (isinitialized) {
            keyb = initialized_value;
            if (slot_offset == 0) {
                return keyb;
            }
            direction = FORWARD;
            goto next;
        }

    prefix_continue:
        if (rip > node->prefixstart && rip <= node->prefixstop) {
            group_key = node->keys.at(node->prefixstart).value;
            string prefixstr = group_key.substr(0, key_prefix);
            keyb = prefixstr + key_data;
            if (slot_offset == 0)
                return (keyb);
            goto switch_and_jump;
        }

        if (direction == BACKWARD) {
            if (slot_offset == 0)
                last_prefix = key_prefix;
            if (slot_offset == 0 || last_prefix > key_prefix) {
                jump_rip = rip;
                jump_slot_offset = slot_offset;
                last_prefix = key_prefix;
            }
        }

        if (direction == FORWARD) {
            keyb = keyb.substr(0, key_prefix) + key_data;
            if (slot_offset == 0)
                break;
        }

    next:
        switch (direction) {
        case BACKWARD:
            --rip;
            ++slot_offset;
            break;
        case FORWARD:
            ++rip;
            --slot_offset;
            break;
        }
    }

    // Initialize the key
    node->keys.at(ind).isinitialized = true;
    node->keys.at(ind).initialized_value = keyb;
    return keyb;
}

// finish
string get_full_key(NodeWT *node, int slot) {
    // Optimization to directly return initialized key value
    string key;
    uint8_t prefix;
    if (node->keys.at(slot).isinitialized) {
        key = node->keys.at(slot).initialized_value;
        prefix = 0;
    }
    else {
        key = node->keys.at(slot).value;
        prefix = node->keys.at(slot).prefix;
    }
    if (key.length() != 0 && prefix == 0) {
        return key;
    }
    if (key.length() != 0 && slot > node->prefixstart && slot <= node->prefixstop) {
        string groupkey = node->keys.at(node->prefixstart).value;
        string prefixstr = groupkey.substr(0, prefix);
        return prefixstr + key;
    }

    else {
        return initialize_prefix_compressed_key(node, slot);
    }
}

#else // no wt cache

// WTitem initialize_prefix_compressed_key_copy(NodeWT *node, int ind) {
//     enum {
//         FORWARD,
//         BACKWARD
//     } direction;
//     WTitem fullkey, orikey;

//     u_int jump_slot_offset = 0;
//     uint8_t last_prefix = 0;
//     uint8_t key_prefix = 0;
//     char* key_data;
//     // string group_key;
//     bool isinitialized;

//     direction = BACKWARD;
//     u_int slot_offset;
//     u_int rip = ind;
//     u_int jump_rip = 0;

//     // string keyb;
//     WThead *header;

//     for (slot_offset = 0;;) {
//         if (0) {
//         switch_and_jump:
//             /* Switching to a forward roll. */
//             direction = FORWARD;

//             /* Skip list of keys with compatible prefixes. */
//             rip = jump_rip;
//             slot_offset = jump_slot_offset;
//         }

//     overflow_retry:

//         header = GetHeaderWT(node, rip);
//         orikey.addr = PageOffset(node, header->key_offset);
//         orikey.size = header->key_len;
//         key_prefix = header->pfx_len;
//         // key_prefix = node->keys.at(rip).prefix;
//         // key_data = node->keys.at(rip).value;

//         // We remove initialized value here, so just do it
//         if (key_prefix == 0) {
//             fullkey = orikey;
//             // keyb = key_data;
//         }
//         else
//             goto prefix_continue;
//         if (slot_offset == 0)
//             return (fullkey);
//         goto switch_and_jump;

//     prefix_continue:
//         /*
//          * Proceed with a prefix-compressed key.
//          *
//          * Prefix compression means we don't yet have a key, but there's a special case: if the key
//          * is part of the group of compressed key prefixes we saved when reading the page into
//          * memory, we can build a key for this slot. Otherwise we have to keep rolling forward or
//          * backward.
//          */
//         if (rip > node->prefixstart && rip <= node->prefixstop) {
//             /*
//              * Get the root key's information (the row-store key can change underfoot; explicitly
//              * take a copy). Ignore the root key's size and prefix information because it must be
//              * large enough (else the current key couldn't have been prefix-compressed based on its
//              * value), and it can't have a prefix-compression value, it's a root key which is never
//              * prefix-compressed.
//              */
//             header = GetHeaderWT(node, node->prefixstart);
//             ;
//             char* newbuf = new char[orikey.size + key_prefix + 1];
//             strncpy(newbuf, PageOffset(node, header->key_offset), key_prefix);
//             strcpy(newbuf + key_prefix, orikey.addr);
//             fullkey.addr = newbuf;
//             fullkey.size = key_prefix + orikey.size;
//             fullkey.newallocated = true;

//             // group_key = node->keys.at(node->prefixstart).value;
//             // string prefixstr = group_key.substr(0, key_prefix);
//             // keyb = prefixstr + key_data;

//             /* If this is the key we originally wanted, we don't care if we're rolling forward
//              * or backward, it's what we want.
//              * The key doesn't need to be instantiated, just return.
//              */
//             if (slot_offset == 0)
//                 return (fullkey);
//             goto switch_and_jump;
//         }
//         /*
//          * 5: an on-page reference to a key that's prefix compressed.
//          * If rolling backward, keep looking for something we can use.
//          * If rolling forward, build the full key and keep rolling forward.
//          */
//         if (direction == BACKWARD) {
//             /*
//              * If there's a set of keys with identical prefixes, we don't want to instantiate each
//              * one, the prefixes are all the same.
//              *
//              * As we roll backward through the page, track the last time the prefix decreased in
//              * size, so we can start with that key during our roll-forward. For a page populated
//              * with a single key prefix, we'll be able to instantiate the key we want as soon as we
//              * find a key without a prefix.
//              */

//             // if (slot_offset == 0)
//             //     last_prefix = key_prefix;

//             // The first key, or the one prefix_len shrinks
//             if (slot_offset == 0 || last_prefix > key_prefix) {
//                 jump_rip = rip;
//                 jump_slot_offset = slot_offset;
//                 last_prefix = key_prefix;
//             }
//         }

//         if (direction == FORWARD) {

//             keyb = keyb.substr(0, key_prefix) + key_data;

//             if (slot_offset == 0)
//                 break;
//         }

//     next:
//         switch (direction) {
//         case BACKWARD:
//             --rip;
//             ++slot_offset;
//             break;
//         case FORWARD:
//             ++rip;
//             --slot_offset;
//             break;
//         }
//     }

//     // Initialize the key
//     node->keys.at(ind).isinitialized = true;
//     node->keys.at(ind).initialized_value = keyb;
//     return keyb;
// }

// Store the decompressed key into key
void get_full_key(NodeWT *node, int idx, WTitem &key) {
    WThead *header = GetHeaderWT(node, idx);
    key.addr = PageOffset(node, header->key_offset);
    key.size = header->key_len;
    int pfx_len = header->pfx_len;

    if (key.size != 0 && pfx_len == 0) { // no prefix
        return;
    }

    char *decomp_key = new char[key.size + pfx_len + 1];
    // Get the compressed prefix from k[base] to k[i]
    // means we need [0:pfx_need - 1] bytes of the prefix, stop when pfx_need == -1
    int prefix_need = pfx_len;
    for (int i = idx - 1; prefix_need > 0 && i >= 0; i--) {
        WThead *head_i = GetHeaderWT(node, i);
        if (head_i->pfx_len < prefix_need) {
            strncpy(decomp_key + head_i->pfx_len,
                    PageOffset(node, head_i->key_offset), prefix_need - head_i->pfx_len);
            prefix_need = head_i->pfx_len;
        }
    }

    strcpy(decomp_key + pfx_len, key.addr);
    key.addr = decomp_key;
    key.size += pfx_len;
    key.newallocated = true;
    return;
}
#endif

// // finish
// WTitem extract_key(NodeWT *node, int idx, bool non_leaf_comp) {
//     // string key = node->keys.at(idx).value;
//     WTitem key;
//     WThead *header = GetHeaderWT(node, idx);
//     // char *key = PageOffset(node, header->key_offset);
//     key.addr = PageOffset(node, header->key_offset);
//     key.size = header->key_len;
//     if (node->IS_LEAF || non_leaf_comp)
//         return get_full_key(node, idx);
//     else {
//         return key;
//     }
// }

// // Apply changes in the following keys in the node
// void build_page_prefixes(NodeWT *node, int pos, string key_at_pos) {
//     if (node->size == 0)
//         return;
//     string prev_key = key_at_pos;
//     uint8_t prev_pfx = node->keys.at(pos).prefix;
//     for (int i = pos + 1; i < node->keys.size(); i++) {
//         string currkey = prev_key.substr(0, node->keys.at(i).prefix) + node->keys.at(i).value;
//         uint8_t curr_pfx = compute_prefix_wt(prev_key, currkey, prev_pfx);
//         node->keys.at(i).value = currkey.substr(curr_pfx);
//         node->keys.at(i).prefix = curr_pfx;
//         prev_pfx = curr_pfx;
//         prev_key = currkey;
//     }
//     record_page_prefix_group(node);
// }

void populate_prefix_backward(NodeWT *node, int pos, char *fullkey_before_pos, int keylen) {
    if (node->size == 0)
        return;
    WThead *header = GetHeaderWT(node, pos);

    uint8_t prev_pfx = header->pfx_len;
    char *prev_fullkey = fullkey_before_pos;
    int prev_len = keylen;

    for (int i = pos + 1; i < node->size; i++) {
        header = GetHeaderWT(node, i);
        int cur_len = header->pfx_len + header->key_len;

        char *fullkey = new char[cur_len + 1];
        strncpy(fullkey, prev_fullkey, header->pfx_len);
        strcpy(fullkey + header->pfx_len, PageOffset(node, header->key_offset));

        uint8_t newpfx_len = compute_new_prefix_len(prev_fullkey, fullkey,
                                                    prev_len, cur_len, prev_pfx);
        // under many cases, newpfx is shorter than prevpfx,
        // then we need to store longer value in the buf
        if (newpfx_len > prev_pfx) {
            // shift the pointer
            header->pfx_len = newpfx_len;
            header->key_len -= newpfx_len - prev_len;
            header->key_offset += newpfx_len - prev_len;
        }
        else if (newpfx_len < prev_pfx) {
            // write new item
            strcpy(BufTop(node), fullkey + newpfx_len);
            header->key_offset = node->space_top;
            header->key_len = cur_len - newpfx_len;
            header->pfx_len = newpfx_len;
            node->space_top += header->key_len + 1;
        }
    }
}

void update_next_prefix(NodeWT *node, int pos, char *fullkey_before_pos,
                        char *full_newkey, int keylen) {
    // if newkey is the last one
    if (node->size <= 1 || pos + 1 == node->size)
        return;
    WThead *header = GetHeaderWT(node, pos + 1);

    int cur_len = header->pfx_len + header->key_len;

    // build k[pos+1] from k[pos - 1]
    char *fullkey = new char[cur_len + 1];
    strncpy(fullkey, fullkey_before_pos, header->pfx_len);
    strcpy(fullkey + header->pfx_len, PageOffset(node, header->key_offset));

    // compare k[pos] and k[pos+1]
    uint8_t newpfx_len = compute_new_prefix_len(full_newkey, fullkey,
                                                keylen, cur_len, header->pfx_len);
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

// inmem_row_leaf
// void record_page_prefix_group(NodeWT *node) {
//     uint32_t best_prefix_count, best_prefix_start, best_prefix_stop;
//     uint32_t last_slot, prefix_count, prefix_start, prefix_stop, slot;
//     uint8_t smallest_prefix;

//     best_prefix_count = prefix_count = 0;
//     smallest_prefix = 0;
//     prefix_start = prefix_stop = 0;
//     last_slot = 0;
//     best_prefix_start = best_prefix_stop = 0;

//     for (int slot = 0; slot < node->keys.size(); slot++) {
//         int prefix = node->keys.at(slot).prefix;
//         if (prefix == 0) {
//             /* If the last prefix group was the best, track it. */
//             if (prefix_count > best_prefix_count) {
//                 best_prefix_start = prefix_start;
//                 best_prefix_stop = prefix_stop;
//                 best_prefix_count = prefix_count;
//             }
//             prefix_count = 0;
//             prefix_start = slot;
//         }
//         else {
//             /* Check for starting or continuing a prefix group. */
//             if (prefix_count == 0 || (last_slot == slot - 1 && prefix <= smallest_prefix)) {
//                 smallest_prefix = prefix;
//                 last_slot = prefix_stop = slot;
//                 ++prefix_count;
//             }
//         }
//     }

//     /* If the last prefix group was the best, track it. Save the best prefix group for the page. */
//     if (prefix_count > best_prefix_count) {
//         best_prefix_start = prefix_start;
//         best_prefix_stop = prefix_stop;
//     }
//     node->prefixstart = best_prefix_start;
//     node->prefixstop = best_prefix_stop;
// }

// string promote_key(NodeWT *node, string lastleft, string firstright) {
//     if (!node->IS_LEAF)
//         return firstright;
//     int len = min(lastleft.length(), firstright.length());
//     int size = len + 1;
//     for (int cnt = 1; len > 0; ++cnt, --len)
//         if (lastleft.at(cnt - 1) != firstright.at(cnt - 1)) {
//             if (size != cnt) {
//                 size = cnt;
//             }
//             break;
//         }
//     return firstright.substr(0, size);
// }

void suffix_truncate(WTitem *lastleft, WTitem *firstright, bool isleaf, WTitem &result) {
    // WTitem separator;
    result.addr = firstright->addr;
    result.newallocated = false;
    if (isleaf) {
        result.size = firstright->size;
    }
    else {
        int common = get_common_prefix_len(lastleft->addr, firstright->addr,
                                           lastleft->size, firstright->size);
        result.size = common + 1;
    }
    return;
}

// string get_uncompressed_key_before_insert(NodeWT *node, int ind, int insertpos, string newkey, bool equal) {
//     if (equal) {
//         return get_full_key(node, ind);
//     }
//     string uncompressed;
//     if (insertpos == ind) {
//         uncompressed = newkey;
//     }
//     else {
//         int origind = insertpos < ind ? ind - 1 : ind;
//         uncompressed = get_full_key(node, origind);
//     }
//     return uncompressed;
// }