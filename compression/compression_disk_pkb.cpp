#pragma once
#include "compression_disk_pkb.h"

int bt_get_parent_child_pos(NodePkB *parent, NodePkB *node) {
    for (int i = 0; i < parent->ptr_cnt; i++) {
        if (parent->ptrs[i] == node)
            return i;
    }
    return -1;
}

void get_base_key_from_ancestor(NodePkB **path, int path_level, NodePkB *node, Item &key) {
    NodePkB *curr = node;
    for (int i = path_level; i >= 0; i--) {
        NodePkB *parent = path[i];
        int nodepos = bt_get_parent_child_pos(parent, curr);

        if (nodepos > 0) {
            PkBhead *head = GetHeaderPkB(parent, nodepos - 1);
            key.addr = PageOffset(parent, head->key_offset);
            key.size = head->key_len;
            return;
        }
        curr = parent;
    }
    key.size = 0;
    return;
}

void generate_pkb_key(NodePkB *node, char *newkey, int keylen, int insertpos,
                      NodePkB **path, int path_level, Item &key) {
    // string basekey;
    if (insertpos > 0) {
        // go to buffer get the complete key

        PkBhead *head = GetHeaderPkB(node, insertpos - 1);
        key.addr = PageOffset(node, head->key_offset);
        key.size = head->key_len;
        return;
    }
    else {
        get_base_key_from_ancestor(path, path_level, node, key);
    }

    // It's enough to return, calculate the offset later
    return;
}

// TODO: see what happens if the new-inserted node is ancestor of a node
void update_next_prefix(NodePkB *node, int pos, char *full_newkey, int keylen) {
    // if newkey is the last one
    if (node->size <= 1 || pos + 1 == node->size)
        return;
    PkBhead *header = GetHeaderPkB(node, pos + 1);

    // compare k[pos] and k[pos+1]
    char *next_k = PageOffset(node, header->key_offset);
    uint8_t newpfx_len = get_common_prefix_len(full_newkey, next_k,
                                               keylen, header->key_len);

    int pk_len = min(header->key_len - newpfx_len, PKB_LEN);
    strncpy(header->pk, next_k + newpfx_len, pk_len);
    header->pk[pk_len] = '\0';
    header->pk_len = pk_len;
    header->pfx_len = newpfx_len;

    return;
}

compareKeyResult compare_key(const char *a, const char *b, int alen, int blen, bool fullcomp) {
    compareKeyResult result;

    int len = min(alen, blen);

    for (int idx = 0; idx < len; ++idx) {
        int cmp = a[idx] - b[idx];
        if (cmp != 0) {
            result.comp = cmp < 0 ? LT : GT;
            result.offset = idx;
            return result;
        }
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
        compareKeyResult partCmpResult = compare_key(searchKey + head->pfx_len, head->pk,
                                                     keylen - head->pfx_len, head->pk_len, false);
        result.offset = head->pfx_len + partCmpResult.offset;
        result.comp = partCmpResult.comp;
        if (result.offset > head->pfx_len + head->pk_len) {
            result.comp = EQ;
        }
    }
    else {
        result.comp = comp;
        result.offset = offset;
    }

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