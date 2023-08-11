#pragma once
#include "compression_wt.h"

uint8_t compute_prefix_wt(string prev_key, string key){
    size_t pfx_max;
    uint8_t pfx = 0;

    pfx_max = UINT8_MAX;
    if (key.length() < pfx_max)
        pfx_max = key.length();
    if (prev_key.length() < pfx_max)
        pfx_max = prev_key.length();

    for (pfx = 0; pfx < pfx_max; ++pfx){
        if (prev_key.at(pfx) != key.at(pfx))
            break;
    }

    return pfx;
}

uint8_t compute_prefix_wt(string prev_key, string key, uint8_t prev_pfx){
    size_t pfx_max;
    uint8_t pfx = 0;
    const char *a, *b;

    pfx_max = UINT8_MAX;
    if (key.length() < pfx_max)
        pfx_max = key.length();
    if (prev_key.length() < pfx_max)
        pfx_max = prev_key.length();

    for (pfx = 0; pfx < pfx_max; ++pfx){
        if (prev_key.at(pfx) != key.at(pfx))
            break;
    }

    if (pfx < prefix_compression_min)
        pfx = 0;
    else if (prev_pfx != 0 && pfx > prev_pfx &&
             pfx < prev_pfx + WT_KEY_PREFIX_PREVIOUS_MINIMUM)
        pfx = prev_pfx;

    return pfx;
}

// finish
string initialize_prefix_compressed_key(NodeWT *node, int ind){
    enum
    {
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

    //Read lock on node for reading initialized values
    shared_lock<shared_mutex> lock(*node->keys.at(ind).mLock);

    for (slot_offset = 0;;){
        if (0){
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

        if (!isinitialized){
            if (key_prefix == 0){
                keyb = key_data;
            } else
                goto prefix_continue;

            if (slot_offset == 0){
                return (keyb);
            }
            goto switch_and_jump;
        }

        if (isinitialized){
            keyb = initialized_value;
            if (slot_offset == 0){
                return keyb;
            }
            direction = FORWARD;
            goto next;
        }

    prefix_continue:
        if (rip > node->prefixstart && rip <= node->prefixstop){
            group_key = node->keys.at(node->prefixstart).value;
            string prefixstr = group_key.substr(0, key_prefix);
            keyb = prefixstr + key_data;
            if (slot_offset == 0)
                return (keyb);
            goto switch_and_jump;
        }

        if (direction == BACKWARD){
            if (slot_offset == 0)
                last_prefix = key_prefix;
            if (slot_offset == 0 || last_prefix > key_prefix){
                jump_rip = rip;
                jump_slot_offset = slot_offset;
                last_prefix = key_prefix;
            }
        }

        if (direction == FORWARD){
            keyb = keyb.substr(0, key_prefix) + key_data;
            if (slot_offset == 0)
                break;
        }

    next:
        switch (direction){
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
    //Upgrade to write lock on node for writing initialized values
    lock.unlock();
    unique_lock<shared_mutex> wlock(*node->keys.at(ind).mLock);
    node->keys.at(ind).isinitialized = true;
    node->keys.at(ind).initialized_value = keyb;
    return keyb;
}

// finish
string get_key(NodeWT *node, int slot){
    // Optimization to directly return initialized key value
    string key;
    uint8_t prefix;
    if (node->keys.at(slot).isinitialized){
        key = node->keys.at(slot).initialized_value;
        prefix = 0;
    }else{
        key = node->keys.at(slot).value;
        prefix = node->keys.at(slot).prefix;
    }
    if (key.length() != 0 && prefix == 0){
        return key;
    }
    if (key.length() != 0 && slot > node->prefixstart && slot <= node->prefixstop){
        string groupkey = node->keys.at(node->prefixstart).value;
        string prefixstr = groupkey.substr(0, prefix);
        return prefixstr + key;
    }

    else {
        return initialize_prefix_compressed_key(node, slot);
    }
}

//finish
string extract_key(NodeWT *node, int ind, bool non_leaf){
    string key = node->keys.at(ind).value;
    if (node->IS_LEAF)
        return get_key(node, ind);
    else{
        if(non_leaf)
            return get_key(node, ind);
        else
            return key;
    }
}

void build_page_prefixes(NodeWT *node, int pos, string key_at_pos){
    if (node->size == 0)
        return;
    string prev_key = key_at_pos;
    uint8_t prev_pfx = node->keys.at(pos).prefix;
    for (int i = pos + 1; i < node->keys.size(); i++){
        string currkey = prev_key.substr(0, node->keys.at(i).prefix) + node->keys.at(i).value;
        uint8_t curr_pfx = compute_prefix_wt(prev_key, currkey, prev_pfx);
        node->keys.at(i).value = currkey.substr(curr_pfx);
        node->keys.at(i).prefix = curr_pfx;
        prev_pfx = curr_pfx;
        prev_key = currkey;
    }
    record_page_prefix_group(node);
}

void record_page_prefix_group(NodeWT *node){
    uint32_t best_prefix_count, best_prefix_start, best_prefix_stop;
    uint32_t last_slot, prefix_count, prefix_start, prefix_stop, slot;
    uint8_t smallest_prefix;

    best_prefix_count = prefix_count = 0;
    smallest_prefix = 0;
    prefix_start = prefix_stop = 0;
    last_slot = 0;
    best_prefix_start = best_prefix_stop = 0;

    for (int slot = 0; slot < node->keys.size(); slot++){
        int prefix = node->keys.at(slot).prefix;
        if (prefix == 0){
            /* If the last prefix group was the best, track it. */
            if (prefix_count > best_prefix_count){
                best_prefix_start = prefix_start;
                best_prefix_stop = prefix_stop;
                best_prefix_count = prefix_count;
            }
            prefix_count = 0;
            prefix_start = slot;
        } else {
            /* Check for starting or continuing a prefix group. */
            if (prefix_count == 0 ||
                (last_slot == slot - 1 && prefix <= smallest_prefix)){
                smallest_prefix = prefix;
                last_slot = prefix_stop = slot;
                ++prefix_count;
            }
        }
    }

    /* If the last prefix group was the best, track it. Save the best prefix group for the page. */
    if (prefix_count > best_prefix_count){
        best_prefix_start = prefix_start;
        best_prefix_stop = prefix_stop;
    }
    node->prefixstart = best_prefix_start;
    node->prefixstop = best_prefix_stop;
}

string promote_key(NodeWT *node, string lastleft, string firstright){
    if (!node->IS_LEAF)
        return firstright;
    int len = min(lastleft.length(), firstright.length());
    int size = len + 1;
    for (int cnt = 1; len > 0; ++cnt, --len)
        if (lastleft.at(cnt - 1) != firstright.at(cnt - 1)){
            if (size != cnt){
                size = cnt;
            }
            break;
        }
    return firstright.substr(0, size);
}

string get_uncompressed_key_before_insert(NodeWT *node, int ind, int insertpos, string newkey){
    string uncompressed;
    if (insertpos == ind){
        uncompressed = newkey;
    }else{
        int origind = insertpos < ind ? ind - 1 : ind;
        uncompressed = get_key(node, origind);
    }
    return uncompressed;
}