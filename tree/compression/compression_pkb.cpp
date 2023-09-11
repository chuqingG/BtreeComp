#pragma once
#include "compression_pkb.h"

int compute_offset(string prev_key, string key) {
    int pfx = 0;
    int pfx_max = min(key.length(), prev_key.length());

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

int bt_get_parent_child_pos(NodePkB *parent, NodePkB *node) {
    for (int i = 0; i < parent->ptrs.size(); i++) {
        if (parent->ptrs.at(i) == node)
            return i;
    }
    return -1;
}

string get_base_key_from_ancestor(vector<NodePkB *> parents, int pos, NodePkB *node) {
    NodePkB *curr = node;
    for (int i = pos; i >= 0; i--) {
        NodePkB *parent = parents.at(i);
        int nodepos = bt_get_parent_child_pos(parent, curr);
        int size = parent->size;
        if (nodepos > 0) {
            return parent->keys[nodepos - 1].original;
        }
        curr = parent;
    }
    return "";
}

KeyPkB generate_pkb_key(NodePkB *node, string newkey, char *newkeyptr, int insertpos, vector<NodePkB *> parents, int parentpos, int rid) {
    string basekey;
    if (insertpos > 0) {
        basekey = get_key(node, insertpos - 1);
    }
    else {
        basekey = get_base_key_from_ancestor(parents, parentpos, node);
    }

    int offset = compute_offset(basekey, newkey);
    return KeyPkB(offset, newkey, newkeyptr, rid);
}

// Build page prefixes from position
void build_page_prefixes(NodePkB *node, int pos) {
    if (node->size == 0)
        return;
    string basekey(node->keys.at(pos).original);
    for (int i = pos + 1; i < node->keys.size(); i++) {
        string currkey(node->keys.at(i).original);
        int offset = compute_offset(basekey, currkey);
        node->keys.at(i).updateOffset(offset);
        basekey = currkey;
    }
}

string get_key(NodePkB *node, int slot) {
    return node->keys.at(slot).original;
}

compareKeyResult compare_key(const char *a, const char *b, int alen, int blen, bool fullcomp) {
    compareKeyResult result;
    // int len, usz, tsz;
    // usz = a.length();
    // tsz = b.length();
    int len = min(alen, blen);
    // int idx = 0;
    for (int idx = 0; idx < len; ++idx) {
        int cmp = a[idx] - b[idx];
        if (cmp != 0) {
            result.comp = cmp < 0 ? LT : GT;
            result.offset = idx;
            return result;
        }
        // string c1(1, a.at(ind));
        // string c2(1, b.at(ind));
        // int cmp = c1.compare(c2);
        // if (cmp != 0) {
        //     result.comp = cmp < 0 ? LT : GT;
        //     result.offset = ind;
        //     return result;
        // }
    }

    if (!fullcomp || alen == blen)
        result.comp = EQ;
    else
        result.comp = (alen > blen) ? GT : LT;
    result.offset = len;
    return result;
}

compareKeyResult compare_part_key(NodePkB *node, int idx,
                                  const char *searchKey, int keylen, cmp comp, int offset) {
    compareKeyResult result;
    PkBhead *head = GetHeaderPkB(node, idx);
    if (head->pfx_len < offset) {
        if (comp == LT)
            result.comp = GT;
        else
            result.comp = LT;
        result.offset = head->pfx_len;
    }
    else if (head->pfx_len == offset) {
        // string partKey = searchKey.substr(0, indKey.offset) + indKey.partialKey;
        // compareKeyResult partCmpResult = compare_key(searchKey, partKey, false);
        // result.offset = partCmpResult.offset;

        compareKeyResult partCmpResult = compare_key(searchKey + head->pfx_len, head->pk,
                                                     keylen - head->pfx_len, head->pk_len, false);
        result.offset = head->pfx_len + partCmpResult.offset;
        result.comp = partCmpResult.comp;
        if (result.offset > head->pfx_len + head->pk_len) {
            result.comp = EQ;
        }
    }

    // result.comp = comp;
    // result.offset = offset;
    return result;
}

findNodeResult find_bit_tree(NodePkB *node, const char *searchKey, int keylen,
                             int low, int high, int offset, bool &equal) {
    findNodeResult result;
    low = low < 0 ? 0 : low;
    high = high == node->size ? high - 1 : high;
    int pos = low;
    int idx = low;
    while (idx <= high) {
        PkBhead *head = GetHeaderPkB(node, idx);
        int currOffset = head->pfx_len;
        char diffBit = head->pk[0];

        if (currOffset >= keylen)
            idx++;
        else {
            // string c1(1, searchKey.at(currOffset));
            // string c2(1, diffBit);
            int cmp = searchKey[currOffset] - diffBit;

            if (cmp >= 0) {
                pos = idx;
                idx++;
            }
            else {
                idx++;
                while (idx <= high) {
                    head = GetHeaderPkB(node, idx);
                    if (head->pfx_len < currOffset)
                        break;
                    idx++;
                }
            }
        }
    }
    PkBhead *head = GetHeaderPkB(node, pos);
    // string indexStr(node->keys.at(pos).original);
    compareKeyResult cmpResult = compare_key(searchKey, PageOffset(node, head->key_offset),
                                             keylen, head->key_len);
    if (cmpResult.comp == EQ) {
        equal = true;
        result.low = pos;
        result.high = pos;
        result.offset = cmpResult.offset;
    }
    // GT case
    else if (cmpResult.comp == GT) {
        int h = pos + 1;
        // while (h <= high && node->keys.at(h).offset > cmpResult.offset)
        //     h++;
        while (h <= high) {
            PkBhead *head_h = GetHeaderPkB(node, h);
            if (head_h->pfx_len <= cmpResult.offset)
                break;
            h++;
        }

        result.low = h - 1;
        result.high = h;
        result.offset = cmpResult.offset;
    }
    // LT case
    else {
        int h = pos;
        result.offset = offset;
        while (h >= low) {
            PkBhead *head_h = GetHeaderPkB(node, h);
            if (head_h->pfx_len <= cmpResult.offset) {
                result.offset = head_h->pfx_len;
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

findNodeResult find_node(NodePkB *node, const char *searchKey, int keylen,
                         int offset, bool &equal) {
    findNodeResult result;
    int high = node->size;
    int low = -1;
    int currOffset = offset;
    cmp currCmp = GT;
    int curr = low + 1;
    while (curr < high) {
        compareKeyResult result = compare_part_key(node, curr, searchKey, keylen, currCmp, currOffset);
        currCmp = result.comp;
        currOffset = result.offset;

        if (currCmp == LT) {
            high = curr;
            break;
        }
        else if (currCmp == GT) {
            low = curr;
            offset = currOffset;
        }
        curr++;
    }
    if (high - low > 1) {
        return find_bit_tree(node, searchKey, keylen, low, high, offset, equal);
    }

    result.low = low;
    result.high = high;
    result.offset = offset;
    return result;
}