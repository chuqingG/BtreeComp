#pragma once
#include "node.h"

#include "../utils/compare.cpp"
void word_conv_store(const char* src, const char* dest);
long word_cmp_loop(char* suffix, int suffixlen, char* key, int keylen);
char* string_conv(const char* key, int keylen, int cutoff);

#define STDGUARD
#define NewPage() (new char[MAX_SIZE_IN_BYTES])
#define SetEmptyPage(p) memset(p, 0, sizeof(char) * MAX_SIZE_IN_BYTES)
#define BufTop(nptr) (nptr->base + nptr->space_top)

#define PageOffset(nptr, off) (char *)(nptr->base + off)

#define UpdateBase(node, newbase) \
    {                             \
        delete[] node->base;      \
        node->base = newbase;     \
    }

#define UpdatePtrs(node, newptrs, num)  \
    {                                   \
        for (int i = 0; i < num; i++)   \
            node->ptrs[i] = newptrs[i]; \
        node->ptr_cnt = num;            \
    }

#define InsertNode(nptr, pos, newnode)                         \
    {                                                          \
        nptr->ptrs.emplace(nptr->ptrs.begin() + pos, newnode); \
        nptr->ptr_cnt += 1;                                    \
    }
/*
================For standard==============
*/
#define UpdatePfxItem(nptr, addr, size, newallo)      \
    {                                                 \
        delete nptr->prefix;                          \
        nptr->prefix = new Item(addr, size, newallo); \
    }

#define GetHeaderStd(nptr, i) (Stdhead *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(Stdhead))
#ifdef UBS
inline void calculateBSMetaData(Node *node) {
    int n = node->size;
    int k = sizeof(int) * 8 - __builtin_clz(n) - 1;
    node->I = (uint16_t) 1 << k;
    int l_aux= n - node->I + 1;
    int l = sizeof(int) * 8 - __builtin_clz(l_aux) - 1 + ((l_aux & (l_aux - 1)) ? 1 : 0);
    node->firstL = 1 << (l - 1);
    node->Ip = (uint16_t) n + 1 - (1 << l);
}
#endif

inline void InsertKeyStd(Node *nptr, int pos, const char *k, uint16_t klen) {
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeaderStd(nptr, i), GetHeaderStd(nptr, i - 1), sizeof(Stdhead));
    }
    // Set the new header
    Stdhead *header = GetHeaderStd(nptr, pos);
    header->key_offset = nptr->space_top;
    #ifdef KN
        if (klen > PV_SIZE) {
            strcpy(BufTop(nptr), k + PV_SIZE);
            nptr->space_top += klen - PV_SIZE + 1;
        }
        else {
            strcpy(BufTop(nptr), "\0");
            nptr->space_top += 1;
        }
        // memset(header->key_prefix, 0, PV_SIZbE);
        memcpy(header->key_prefix, k, PV_SIZE);
    #else
        strcpy(BufTop(nptr), k);
        nptr->space_top += klen + 1;
    #endif

    header->key_len = klen;
    nptr->size += 1;
#ifdef UBS
    calculateBSMetaData(nptr);
#endif
}

// TODO:
inline void RemoveKeyStd(Node *nptr, int pos, const char *k, uint16_t klen) {
    strcpy(BufTop(nptr), k);
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeaderStd(nptr, i), GetHeaderStd(nptr, i - 1), sizeof(Stdhead));
    }
    // Set the new header
    Stdhead *header = GetHeaderStd(nptr, pos);
    header->key_offset = nptr->space_top;
    header->key_len = klen;
    nptr->space_top += klen + 1;
    nptr->size += 1;
}

// with cutoff
inline void CopyToNewPageStd(Node *nptr, int low, int high, char *newbase, uint16_t cutoff, uint16_t &top) {//cutoff is potential head_comp ignored bytes
    for (int i = low; i < high; i++) {
        int newidx = i - low;
        Stdhead *oldhead = GetHeaderStd(nptr, i);
        Stdhead *newhead = (Stdhead *)(newbase + MAX_SIZE_IN_BYTES
                                       - (newidx + 1) * sizeof(Stdhead));
        int key_len = oldhead->key_len;
#ifdef PV
            int l = oldhead->key_len < PV_SIZE ? PV_SIZE : oldhead->key_len;
            char *presuf = new char[l + 1]; //extract entire key
            presuf[l] = '\0';
            memcpy(presuf, oldhead->key_prefix, PV_SIZE);
            if (key_len > PV_SIZE) strncpy(presuf + PV_SIZE, PageOffset(nptr, oldhead->key_offset), key_len - PV_SIZE);

            newhead->key_len = oldhead->key_len - cutoff;
            newhead->key_offset = top;
            memset(newhead->key_prefix, 0, PV_SIZE); //cutoff can't be longer than length right? yes
#ifdef KN
            char *forward = string_conv(presuf, l, 0); //convert forward
            char *backward = string_conv(forward, l, cutoff); delete[] forward;//and back with cutoff
            memcpy(newhead->key_prefix, backward, PV_SIZE); //at least PV_SIZE long. cutoff not in presuf
            int sufLength = oldhead->key_len - cutoff - PV_SIZE; if (sufLength < 0) sufLength = 0; //for nullbyte
            strncpy(newbase + top, backward + PV_SIZE, sufLength); //ends at nullbyte, even if 0
            delete[] backward;
#else
            strcpy(newhead->key_prefix, presuf + cutoff, min(PV_SIZE, (int)newhead->key_len));
            int sufLength = oldhead->key_len - cutoff - PV_SIZE; if (sufLength < 0) sufLength = 0;
            strncpy(newbase + top, presuf + cutoff + PV_SIZE, sufLength); //ends at nullbyte, even if 0
#endif
            top += sufLength + 1; //if key can fit into prefix, then there will be a null_byte place holder
            delete[] presuf;
#else
            strcpy(BufTop(nptr), k);
            strcpy(newbase + top, PageOffset(nptr, oldhead->key_offset) + cutoff);
            newhead->key_len = oldhead->key_len - cutoff;
            newhead->key_offset = top;
            top += newhead->key_len + 1;
#endif
        // if (newhead->key_len > 32)
        //     cout << "wrong update" << endl;
    }
}

#ifdef PV

inline void word_conv_store(char* src, char* dest) { //int length only for now
    char c3 = src[3]; //supports in-place
    char c2 = src[2];
    dest[3] = src[0];
    dest[0] = c3;
    dest[2] = src[1];
    dest[1] = c2;
}

inline char* string_conv(const char* key, int keylen, int cutoff) {//unnormalized to normalized
    keylen -= cutoff;
    key += cutoff;
    if (keylen <= 0) return "\0";
    if (keylen <= PV_SIZE) { //keylen
        char* result = new char[PV_SIZE + 1];
        memset(result, 0, PV_SIZE + 1); //nullbyte
        memcpy(result, key, min(keylen, PV_SIZE));
        word_conv_store(result, result);
        return result;
    }
    char *result = new char[keylen + 1];
    int originalKeylen = keylen;
    char *pointer = result;
    while (keylen >= PV_SIZE) {
        word_conv_store((char*)key, pointer);
        keylen -= PV_SIZE;
        pointer += PV_SIZE;
        key += PV_SIZE;
    }
    for (int i = 0; i < keylen; i++) {
        pointer[i] = key[i];//should cover s
    }
    result[originalKeylen] = '\0';
    return result;
}

inline long word_cmp(Stdhead* header,const char* key, int keylen) {
    if (keylen < PV_SIZE) { //key already normalized
        char word[5] = {0};
        memcpy(word, key, PV_SIZE);
         return *(int*)word - *(int*)header->key_prefix;
    }
#if PV_SIZE == 4
    return *(int*)key - *(int*)header->key_prefix;
#else 
    return *(long*)key - *(long*)header->key_prefix;
#endif
}

inline long pvComp(Stdhead* header,const char* key, int keylen, Node *cursor) {
    long cmp = word_cmp(header, key, keylen);
    if (cmp == 0 && keylen > PV_SIZE && header->key_len > PV_SIZE) {
#ifdef KN
        cmp = word_cmp_loop(PageOffset(cursor, header->key_offset), header->key_len - PV_SIZE, (char*)key + PV_SIZE, keylen - PV_SIZE);
#else
        cmp = char_cmp_new(key, PageOffset(cursor, header->key_offset),
                            keylen, header->key_len);
#endif
    }
    return cmp;
}

inline long word_cmp_loop(char* suffix, int suffixlen, char* key, int keylen) {
    long cmp = 0;
    int numOfWords = min(suffixlen, keylen) / PV_SIZE;
    for (int i = 0; i < numOfWords; i++) {
#if PV_SIZE == 4
        cmp = *(int*)key - *(int*)suffix;
#else 
        cmp = *(long*)key - *(long*)suffix;
#endif
        if (cmp != 0) return cmp;
        suffix += PV_SIZE;
        key += PV_SIZE;
    }
    suffixlen -= numOfWords * PV_SIZE;
    keylen -= numOfWords * PV_SIZE;


    // int idx = *matchp;
    if (keylen == suffixlen) {//both unnorm
        int cmp_len = min(keylen, suffixlen);
        for (int idx = 0; idx < cmp_len; ++idx) {
            cmp = key[idx] - suffix[idx];
            if (cmp != 0)
                return cmp;
        }
    }
    else { //one is unnorm, the other is norm
        int cmp_len = min(keylen, suffixlen);
        for (int idx = 0; idx < cmp_len; ++idx) {
            cmp = key[ keylen < suffixlen ? idx : PV_SIZE - 1 - idx] - suffix[ keylen < suffixlen ? PV_SIZE - 1 - idx : idx];
            if (cmp != 0)
                return cmp;
        }
    }
    return keylen - suffixlen;
}

#endif
#ifdef UBS
inline int unrolledBinarySearch(Node *cursor, const char *key, int keylen, long &cmp) {//cutoff is potential head_comp ignored bytes
    // if (cursor->size <= 16) {
    //     int i;
    //     for (i = 0; i < cursor->size; i++) {
    //         Stdhead *ki = GetHeaderStd(cursor, i);
    //         cmp = pvComp(ki, key, keylen, cursor);
    //         if (cmp < 0) return i;
    //     }
    //     return cursor->size - 1; //key is the larger than all keys
    // }
    int curPos = cursor->I - 1; //2^k, where k is floor(log cursor->size);
    Stdhead *ki = GetHeaderStd(cursor, curPos);
    uint16_t delta = cursor->I;

    cmp = pvComp(ki, key, keylen, cursor); //initial probe cost
    delta = delta >> 1;
    if (cmp == 0 || delta == 0) return curPos;
    else if (cmp > 0) { //if K > Ki
            curPos = cursor->Ip - 1;
            delta = cursor->firstL;
            curPos += delta;
    }
    else curPos -= delta;

    while (delta != 0) {
        ki = GetHeaderStd(cursor, curPos); //2
        cmp = pvComp(ki, key, keylen, cursor); 
        delta /= 2;
        if (cmp == 0) return curPos;
        else if (cmp > 0) curPos += delta;
        else curPos -= delta;
    }
    return curPos;
}
#endif
/*
===============For DB2=============
*/
#define NewPageDB2() (new char[MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE])
#define SetEmptyPageDB2(p) memset(p, 0, sizeof(char) * (MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE))

#define GetKeyDB2(result, off) (char *)(result->base + off)
#define GetPfxDB2(result, off) (char *)(result->base + MAX_SIZE_IN_BYTES + off)
#define GetHeaderInPageDB2(result, i) (DB2head *)(result->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(DB2head))
#define GetPfxInPageDB2(result, i) (DB2pfxhead *)(result->base + (MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE) - (i + 1) * sizeof(DB2pfxhead))

#define PfxTop(nptr) (nptr->base + MAX_SIZE_IN_BYTES + nptr->pfx_top)

#define GetHeaderDB2(nptr, i) (DB2head *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(DB2head))

#define GetHeaderDB2pfx(nptr, i) (DB2pfxhead *)(nptr->base + MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE - (i + 1) * sizeof(DB2pfxhead))

#define PfxOffset(node, off) (char *)(node->base + MAX_SIZE_IN_BYTES + off)

inline void InsertPfxDB2(NodeDB2 *nptr, int pos, const char *p, uint16_t plen, uint16_t low, uint16_t high) {
    char *temp = PfxTop(nptr);
    memcpy(temp, p, sizeof(char) * plen);
    temp[plen] = '\0';
    // shift the headers
    for (int i = nptr->pfx_size; i > pos; i--) {
        memcpy(GetHeaderDB2pfx(nptr, i), GetHeaderDB2pfx(nptr, i - 1), sizeof(DB2pfxhead));
    }
    // Set the new header
    DB2pfxhead *header = GetHeaderDB2pfx(nptr, pos);
    header->pfx_offset = nptr->pfx_top;
    header->pfx_len = plen;
    header->low = low;
    header->high = high;
    nptr->pfx_top += plen + 1;
    nptr->pfx_size += 1;
}

inline void InsertKeyDB2(NodeDB2 *nptr, int pos, const char *k, uint16_t klen) {
    strcpy(BufTop(nptr), k);
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeaderDB2(nptr, i), GetHeaderDB2(nptr, i - 1), sizeof(DB2head));
    }
    // Set the new header
    DB2head *header = GetHeaderDB2(nptr, pos);
    header->key_offset = nptr->space_top;
    header->key_len = klen;
    nptr->space_top += klen + 1;
    nptr->size += 1;
}

inline void CopyToNewPageDB2(NodeDB2 *nptr, int low, int high, char *newbase, uint16_t &top) {
    for (int i = low; i < high; i++) {
        int newidx = i - low;
        DB2head *oldhead = GetHeaderDB2(nptr, i);
        DB2head *newhead = (DB2head *)(newbase + MAX_SIZE_IN_BYTES
                                       - (newidx + 1) * sizeof(DB2head));
        strncpy(newbase + top, PageOffset(nptr, oldhead->key_offset), oldhead->key_len);
        newhead->key_len = oldhead->key_len;
        newhead->key_offset = top;
        top += oldhead->key_len + 1;
    }
}

// for a single base without a node scope
#define WriteKeyDB2Page(base, memusage, pos, k, klen, plen)                                     \
    {                                                                                           \
        strcpy(base + memusage, k + plen);                                                      \
        DB2head *head = (DB2head *)(newbase + MAX_SIZE_IN_BYTES - (pos + 1) * sizeof(DB2head)); \
        head->key_len = klen - plen;                                                            \
        head->key_offset = memusage;                                                            \
        memusage += head->key_len + 1;                                                          \
    }

// Only write to the end of prefix page
// the input pfxbase mush be based on node->base + max_size_in_byte
#define WritePfxDB2Page(base, pfxtop, pfxitem, pfxsize)                                                                      \
    {                                                                                                                        \
        strcpy(base + MAX_SIZE_IN_BYTES + pfxtop, pfxitem.prefix.addr);                                                      \
        DB2pfxhead *head = (DB2pfxhead *)(base + MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE - (pfxsize + 1) * sizeof(DB2pfxhead)); \
        head->pfx_offset = pfx_top;                                                                                          \
        head->pfx_len = pfxitem.prefix.size;                                                                                 \
        head->low = pfxitem.low;                                                                                             \
        head->high = pfxitem.high;                                                                                           \
        pfxtop += pfxitem.prefix.size + 1;                                                                                   \
        pfxsize++;                                                                                                           \
    }
/*
===============For WiredTiger=============
*/

// Get the ith header, i starts at 0
#define GetHeaderWT(nptr, i) (WThead *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(WThead))

// The prefix should be cutoff before calling this
inline void InsertKeyWT(NodeWT *nptr, int pos, const char *k, int klen, int plen) {
    strcpy(BufTop(nptr), k);
    // shift the headers
    for (int i = nptr->size; i > pos; i--) {
        memcpy(GetHeaderWT(nptr, i), GetHeaderWT(nptr, i - 1), sizeof(WThead));
    }
    // Set the new header
    WThead *header = GetHeaderWT(nptr, pos);
    header->key_offset = nptr->space_top;
    header->key_len = (uint16_t)klen;
    header->pfx_len = (uint16_t)plen;
#ifdef WTCACHE
    header->initialized = false;
#endif
    nptr->space_top += klen + 1;
    nptr->size += 1;
}

inline void CopyToNewPageWT(NodeWT *nptr, int low, int high, char *newbase, int &top) {
    for (int i = low; i < high; i++) {
        int newidx = i - low;
        WThead *oldhead = GetHeaderWT(nptr, i);
        WThead *newhead = (WThead *)(newbase + MAX_SIZE_IN_BYTES
                                     - (newidx + 1) * sizeof(WThead));
        strncpy(newbase + top, PageOffset(nptr, oldhead->key_offset), oldhead->key_len);
        newhead->key_len = (uint16_t)oldhead->key_len;
        newhead->key_offset = top;
        newhead->pfx_len = (uint16_t)oldhead->pfx_len;
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
    header->key_len = (uint16_t)klen;
    header->pfx_len = (uint16_t)plen;
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
        newhead->key_len = (uint16_t)oldhead->key_len;
        newhead->key_offset = top;
        newhead->pfx_len = (uint16_t)oldhead->pfx_len;
        top += oldhead->key_len + 1;
    }
}

#define GetHeaderPkB(nptr, i) (PkBhead *)(nptr->base + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(PkBhead))

// the k and klen here should always be the fullkey
inline void InsertKeyPkB(NodePkB *nptr, int pos, const char *k, uint16_t klen, uint16_t plen) {
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