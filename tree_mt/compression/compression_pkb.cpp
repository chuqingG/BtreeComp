#pragma once
#include "compression_pkb.h"

int compute_offset(string prev_key, string key){
    int pfx = 0;
    int pfx_max = min(key.length(), prev_key.length());

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

int bt_get_parent_child_pos(NodePkB *parent, NodePkB *node){
    for (int i = 0; i < parent->ptrs.size(); i++){
        if (parent->ptrs.at(i) == node)
            return i;
    }
    return -1;
}

KeyPkB generate_pkb_key(NodePkB *node, string newkey, char * newkeyptr, int insertpos, vector<NodePkB *> parents, int parentpos, int rid){
    string basekey;
    if (insertpos > 0){
        basekey = get_key(node, insertpos - 1);
    }else{
        basekey = node->lowkey;
    }

    int offset = compute_offset(basekey, newkey);
    return KeyPkB(offset, newkey, newkeyptr, rid);
}

// Build page prefixes from position
void build_page_prefixes(NodePkB *node, int pos){
    if (node->size == 0)
        return;
    string basekey(node->keys.at(pos).original);
    for (int i = pos + 1; i < node->keys.size(); i++){
        string currkey(node->keys.at(i).original);
        int offset = compute_offset(basekey, currkey);
        node->keys.at(i).updateOffset(offset);
        basekey = currkey;
    }
}

string get_key(NodePkB *node, int slot){
    return node->keys.at(slot).original;
}

compareKeyResult compare_key(string a, string b, bool fullcomp){
    compareKeyResult result;
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
            result.comp = cmp < 0 ? LT : GT;
            result.offset = ind;
            return result;
        }
    }

    if (!fullcomp || usz == tsz)
        result.comp = EQ;
    else
        result.comp = (usz > tsz) ? GT : LT;
    result.offset = min(usz, tsz);
    return result;
}

compareKeyResult compare_part_key(string searchKey, KeyPkB indKey, cmp comp, int offset){
    compareKeyResult result;
    if (indKey.offset < offset){
        if (comp == LT)
            comp = GT;
        else
            comp = LT;

        offset = indKey.offset;
    }else if (indKey.offset == offset){
        string partKey = searchKey.substr(0, indKey.offset) + indKey.partialKey;
        compareKeyResult partCmpResult = compare_key(searchKey, partKey, false);
        offset = partCmpResult.offset;
        comp = partCmpResult.comp;
        if (offset > indKey.offset + indKey.pkLength){
            comp = EQ;
        }
    }

    result.comp = comp;
    result.offset = offset;
    return result;
}

findNodeResult find_bit_tree(NodePkB *node, string searchKey, int low, int high, int offset, bool &equal){
    findNodeResult result;
    low = low < 0 ? 0 : low;
    high = high == node->size ? high - 1 : high;
    int pos = low;
    int ind = low;
    while (ind <= high){
        int currOffset = node->keys.at(ind).offset;
        char diffBit = node->keys.at(ind).partialKey[0];

        if (currOffset >= searchKey.length())
            ind++;
        else{
            string c1(1, searchKey.at(currOffset));
            string c2(1, diffBit);
            int cmp = c1.compare(c2);

            if (cmp >= 0){
                pos = ind;
                ind++;
            }else{
                ind++;
                while (ind <= high && node->keys.at(ind).offset >= currOffset)
                    ind++;
            }
        }
    }
    string indexStr(node->keys.at(pos).original);
    compareKeyResult cmpResult = compare_key(searchKey, indexStr);
    if (cmpResult.comp == EQ){
        equal = true;
        result.low = pos;
        result.high = pos;
        result.offset = cmpResult.offset;
    }
    // GT case
    else if (cmpResult.comp == GT){
        int h = pos + 1;
        while (h <= high && node->keys.at(h).offset > cmpResult.offset)
            h++;
        result.low = h - 1;
        result.high = h;
        result.offset = cmpResult.offset;
    }
    // LT case
    else{
        int h = pos;
        result.offset = offset;
        while (h >= low){
            if (node->keys.at(h).offset <= cmpResult.offset){
                result.offset = node->keys.at(h).offset;
                h--;
                break;
            }
            h--;
        }
        result.low = h;
        result.high = cmpResult.comp == EQ ? h : h + 1;
    }
    return result;
}

findNodeResult find_node(NodePkB *node, string searchKey, int offset, bool &equal){
    findNodeResult result;
    int high = node->size;
    int low = -1;
    int currOffset = offset;
    cmp currCmp = GT;
    int curr = low + 1;
    while (curr < high){
        compareKeyResult result = compare_part_key(searchKey, node->keys.at(curr), currCmp, currOffset);
        currCmp = result.comp;
        currOffset = result.offset;

        if (currCmp == LT){
            high = curr;
            break;
        }else if (currCmp == GT){
            low = curr;
            offset = currOffset;
        }
        curr++;
    }
    if (high - low > 1){
        return find_bit_tree(node, searchKey, low, high, offset, equal);
    }

    result.low = low;
    result.high = high;
    result.offset = offset;
    return result;
}