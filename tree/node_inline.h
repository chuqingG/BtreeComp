#pragma once
#include "node.h"
#include "../include/config.h"

#define NewPage() (char *)malloc(MAX_SIZE_IN_BYTES * sizeof(char))
#define SetEmptyPage(p) memset(p, 0, sizeof(char) * MAX_SIZE_IN_BYTES)

#define UpdateBase(node, newbase) \
    {                             \
        delete node->base;        \
        node->base = newbase;     \
    }

#define UpdatePtrs(node, newptrs, num)  \
    {                                   \
        for (int i = 0; i < num; i++)   \
            node->ptrs[i] = newptrs[i]; \
        node->ptr_cnt = num;            \
    }

#define UpdateSize(node, newsize)  \
    \ 
{                             \
        delete node->keys_size;    \
        node->keys_size = newsize; \
    }

#define UpdateOffset(node, newoffset)  \
    {                                  \
        delete node->keys_offset;      \
        node->keys_offset = newoffset; \
    }

#define CopySize(node, newsize)                                           \
    \ 
{                                                                    \
        memcpy(node->keys_size, newsize, sizeof(uint8_t) * kNumberBound); \
    }

#define CopyOffset(node, newoffset)                                            \
    {                                                                          \
        memcpy(node->keys_offset, newoffset, sizeof(uint16_t) * kNumberBound); \
    }

#define GetKey(nptr, idx) (char *)(nptr->base + nptr->keys_offset[idx])

#define PageTail(nptr) nptr->base + nptr->memusage

#define InsertOffset(nptr, pos, offset)                      \
    {                                                        \
        for (int i = nptr->size; i > pos; i--)               \
            nptr->keys_offset[i] = nptr->keys_offset[i - 1]; \
        nptr->keys_offset[pos] = (uint16_t)offset;           \
    }

#define InsertSize(nptr, pos, len)                       \
    {                                                    \
        for (int i = nptr->size; i > pos; i--)           \
            nptr->keys_size[i] = nptr->keys_size[i - 1]; \
        nptr->keys_size[pos] = (uint8_t)len;             \
    }

// #define InsertNode(nptr, pos, newnode)         \
//     {                                          \
//         for (int i = nptr->size; i > pos; i--) \
//             nptr->ptrs[i] = nptr->ptrs[i - 1]; \
//         nptr->ptrs[pos] = newnode;             \
//         nptr->ptr_cnt += 1; \
//     }
// #define InsertSize(nptr, pos, len) \
//     nptr->keys_size.emplace(nptr->keys_size.begin() + pos, len)

#define InsertNode(nptr, pos, newnode)                         \
    {                                                          \
        nptr->ptrs.emplace(nptr->ptrs.begin() + pos, newnode); \
        nptr->ptr_cnt += 1;                                    \
    }

// Insert k into nptr[pos]
#define InsertKey(nptr, pos, k, klen)            \
    {                                            \
        strcpy(PageTail(nptr), k);               \
        InsertOffset(nptr, pos, nptr->memusage); \
        InsertSize(nptr, pos, klen);             \
        nptr->memusage += klen + 1;              \
        nptr->size += 1;                         \
    }

// Copy node->keys[low, high) to Page(base, mem, idx)

#define CopyKeyToPage(node, low, high, base, mem, idx, size) \
    for (int i = low; i < high; i++) {                       \
        char *k = GetKey(node, i);                           \
        int klen = node->keys_size[i];                       \
        strcpy(base + mem, k);                               \
        idx[i - (low)] = mem;                                \
        size[i - (low)] = klen;                              \
        mem += klen + 1;                                     \
    }

#define WriteKeyDB2Page(base, memusage, pos, size, idx, kptr, klen, prefixlen) \
    {                                                                          \
        strcpy(base + memusage, kptr + prefixlen);                             \
        size[pos] = klen - prefixlen;                                          \
        idx[pos] = memusage;                                                   \
        memusage += size[pos] + 1;                                             \
    }

#define GetKeyDB2ByPtr(resultptr, i) (char *)(resultptr->base + resultptr->newoffset[i])
#define GetKeyDB2(result, i) (char *)(result.base + result.newoffset[i])

/*
===============For WiredTiger=============
*/
#define BufTop(nptr) (nptr->base + nptr->space_top)
#define GetNewHeader(nptr) (WThead *)(nptr->base + nptr->space_bottom - sizeof(WThead))

// Get the ith header, i starts at 0
#define GetHeader(nptr, i) (WThead *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(WThead))

#define PageOffset(nptr, off) (char *)(nptr->base + off)

// TODO: think whether need to maintain space_bottom
// The prefix should be cutoff before calling this
inline void InsertKeyWT(NodeWT *nptr, int pos, const char *k, int klen, int plen) {
    strcpy(BufTop(nptr), k);
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeader(nptr, i), GetHeader(nptr, i - 1), sizeof(WThead));
    }
    // Set the new header
    WThead *header = GetHeader(nptr, pos);
    header->key_offset = nptr->space_top;
    header->key_len = (uint8_t)klen;
    header->pfx_len = (uint8_t)plen;
#ifdef WTCACHE
    header->initialized = false;
#endif
    nptr->space_top += klen + 1;
    nptr->size += 1;
}

inline void CopyToNewPageWT(NodeWT *nptr, int low, int high, char *newbase, int &top) {
    for (int i = low; i < high; i++) {
        int newidx = i - low;
        WThead *oldhead = GetHeader(nptr, i);
        WThead *newhead = (WThead *)(newbase + MAX_SIZE_IN_BYTES
                                     - (newidx + 1) * sizeof(WThead));
        strncpy(newbase + top, PageOffset(nptr, oldhead->key_offset), oldhead->key_len);
        newhead->key_len = (uint8_t)oldhead->key_len;
        newhead->key_offset = top;
        newhead->pfx_len = (uint8_t)oldhead->pfx_len;
        top += oldhead->key_len + 1;
    }
}

#define GetHeaderMyISAM(nptr, i) (MyISAMhead *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(MyISAMhead))

inline void InsertKeyMyISAM(NodeMyISAM *nptr, int pos, const char *k, int klen, int plen) {
    strcpy(BufTop(nptr), k);
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeaderMyISAM(nptr, i), GetHeaderMyISAM(nptr, i - 1), sizeof(MyISAMhead));
    }
    // Set the new header
    MyISAMhead *header = GetHeaderMyISAM(nptr, pos);
    header->key_offset = nptr->space_top;
    header->key_len = (uint8_t)klen;
    header->pfx_len = (uint8_t)plen;
    nptr->space_top += klen + 1;
    nptr->size += 1;
}

inline void CopyToNewPageMyISAM(NodeMyISAM *nptr, int low, int high, char *newbase, int &top) {
    for (int i = low; i < high; i++) {
        int newidx = i - low;
        MyISAMhead *oldhead = GetHeaderMyISAM(nptr, i);
        MyISAMhead *newhead = (MyISAMhead *)(newbase + MAX_SIZE_IN_BYTES
                                             - (newidx + 1) * sizeof(MyISAMhead));
        strncpy(newbase + top, PageOffset(nptr, oldhead->key_offset), oldhead->key_len);
        newhead->key_len = (uint8_t)oldhead->key_len;
        newhead->key_offset = top;
        newhead->pfx_len = (uint8_t)oldhead->pfx_len;
        top += oldhead->key_len + 1;
    }
}

#define GetHeaderPkB(nptr, i) (PkBhead *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(PkBhead))

// the k and klen here should always be the fullkey
inline void InsertKeyPkB(NodePkB *nptr, int pos, const char *k, uint8_t klen, uint8_t plen) {
    strcpy(BufTop(nptr), k);
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeaderPkB(nptr, i), GetHeaderPkB(nptr, i - 1), sizeof(PkBhead));
    }
    // Set the new header
    PkBhead *header = GetHeaderPkB(nptr, pos);
    header->key_offset = nptr->space_top;
    header->key_len = klen;
    header->pfx_len = plen;
    if (plen < klen) {
        int pk_len = min(klen - plen, PKB_LEN);
        strncpy(header->pk, k + plen, pk_len);
        header->pk[pk_len] = '\0';
        header->pk_len = pk_len;
    }
    else {
        memset(header->pk, 0, sizeof(header->pk));
        header->pk_len = 0;
    }

    nptr->space_top += klen + 1;
    nptr->size += 1;
}

inline void CopyToNewPagePkB(NodePkB *nptr, int low, int high, char *newbase, int &top) {
    for (int i = low; i < high; i++) {
        int newidx = i - low;
        PkBhead *oldhead = GetHeaderPkB(nptr, i);
        PkBhead *newhead = (PkBhead *)(newbase + MAX_SIZE_IN_BYTES
                                       - (newidx + 1) * sizeof(PkBhead));
        strncpy(newbase + top, PageOffset(nptr, oldhead->key_offset), oldhead->key_len);
        memcpy(newhead, oldhead, sizeof(PkBhead));
        // offset is different
        newhead->key_offset = top;
        top += oldhead->key_len + 1;
    }
}
