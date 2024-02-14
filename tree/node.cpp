#pragma once
#include "node.h"
#include "node_inline.h"

// Constructor of Node
Node::Node() {
    size = 0;
    ptr_cnt = 0;
    space_top = 0;
    invalid_len = 0;
    base = NewPage();
    SetEmptyPage(base);

    lowkey = new Item(true);
    highkey = new Item(); // this hk will be deallocated automatically
    // highkey->addr = new char[9];
    // strcpy(highkey->addr, MAXHIGHKEY);
    // highkey->newallocated = true;
    highkey->size = 0;
    prefix = new Item(true);
    IS_LEAF = true;
    prev = nullptr;
    next = nullptr;
}

// Destructor of Node
Node::~Node() {
    delete lowkey;
    delete highkey;
    delete prefix;
    delete[] base;
}

int findk(Node *n, const char *key, int keylen) {
    // return the pos of the key, 0 for not found, size for this node too small
    int low = 0, high = n->size;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Stdhead *header = GetHeaderStd(n, mid);

        int cmp = char_cmp_new(key, PageOffset(n, header->key_offset),
                               keylen, header->key_len);
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return n->IS_LEAF ? -1 : high + 1;
}

void Node::erase(const char *k, int keylen, int idx = -1) {
    if (idx < 0) // uninitialized
        idx = findk(this, k, keylen);
    if (idx < 0)
        cout << "[Not found] The key doesn't exist in the tree" << endl;
    if (idx < this->size) {
        // Found the key in the current node
        if (IS_LEAF) {
            // Remove from the leaf
            RemoveKeyStd(this, idx, keylen);
        }
        else {
            // Remove from the nonleaf node
            if (ptrs[idx]->invalid_len >= ptrs[idx]->space_top / 2) {
                // get predecessor
                Node *cur = ptrs[idx];
                while (!cur->IS_LEAF) {
                    cur = cur->ptrs[cur->size];
                }
                Stdhead *h_pred = GetHeaderStd(cur, cur->size - 1);
                Stdhead *h_old = GetHeaderStd(this, idx);
                char *k_pred = PageOffset(cur, h_pred->key_offset);
                if (keylen >= h_pred->key_len) {
                    memmove(PageOffset(this, h_old->key_offset),
                            k_pred, sizeof(char) * h_pred->key_len + 1);
                }
                else {
                    cout << "WARNING: no enough space to update" << endl;
                    return;
                }
                ptrs[idx]->erase(k_pred, h_pred->key_len);
            }
            else if (ptrs[idx + 1]->invalid_len >= ptrs[idx + 1]->space_top / 2) {
                // get successor
                Node *cur = ptrs[idx + 1];
                while (!cur->IS_LEAF) {
                    cur = cur->ptrs[0];
                }
                Stdhead *h_succ = GetHeaderStd(cur, 0);
                Stdhead *h_old = GetHeaderStd(this, idx);
                char *k_succ = PageOffset(cur, h_succ->key_offset);
                if (keylen >= h_succ->key_len) {
                    memmove(PageOffset(this, h_old->key_offset),
                            k_succ, sizeof(char) * h_succ->key_len + 1);
                }
                else {
                    cout << "WARNING: no enough space to update" << endl;
                    return;
                }
                ptrs[idx]->erase(k_succ, h_succ->key_len);
            }
            else {
                // 1.merge 2. remove
                ptrs[idx]->erase(k, keylen);
            }
        }
    }
    else {
        bool lastchild = idx == this->size;
    }
}

//===============Below for DB2===========

NodeDB2::NodeDB2() {
    size = 0;
    pfx_size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPageDB2();
    SetEmptyPageDB2(base);
    IS_LEAF = true;
    space_top = 0;
    pfx_top = 0;

    ptr_cnt = 0;
    InsertPfxDB2(this, 0, "", 0, 0, 0);
}

// Destructor of NodeDB2
NodeDB2::~NodeDB2() {
    delete[] base;
}

/*
==================For WiredTiger================
*/

NodeWT::NodeWT() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;

    prefixstart = 0;
    prefixstop = 0;

    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
    IS_LEAF = true;
}

// Destructor of NodeWT
NodeWT::~NodeWT() {
    delete[] base;
}

NodeMyISAM::NodeMyISAM() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
    IS_LEAF = true;
}

// Destructor of NodeMyISAM
NodeMyISAM::~NodeMyISAM() {
    delete[] base;
}

NodePkB::NodePkB() {
    size = 0;
    prev = nullptr;
    next = nullptr;
    ptr_cnt = 0;
    base = NewPage();
    SetEmptyPage(base);
    space_top = 0;
    IS_LEAF = true;
}

// Destructor of NodePkB
NodePkB::~NodePkB() {
    delete[] base;
}

void printKeys(Node *node, bool compressed) {
    if (compressed && node->prefix->addr)
        cout << node->prefix->addr << ": ";
    for (int i = 0; i < node->size; i++) {
        Stdhead *head = GetHeaderStd(node, i);
        if (compressed && node->prefix->addr) {
            cout << PageOffset(node, head->key_offset) << ",";
        }
        else {
            cout << node->prefix->addr << PageOffset(node, head->key_offset) << ",";
        }
    }
}

void printKeys_db2(NodeDB2 *node, bool compressed) {
    for (int i = 0; i < node->pfx_size; i++) {
        DB2pfxhead *pfx = GetHeaderDB2pfx(node, i);
        if (compressed) {
            cout << "Prefix " << PfxOffset(node, pfx->pfx_offset) << ": ";
        }
        for (int l = pfx->low; l <= pfx->high; l++) {
            // Loop through rid list to print duplicates
            DB2head *head = GetHeaderDB2(node, l);
            if (compressed) {
                cout << PageOffset(node, head->key_offset) << ",";
            }
            else {
                cout << PfxOffset(node, pfx->pfx_offset) << PageOffset(node, head->key_offset) << ",";
            }
        }
    }
}

void printKeys_myisam(NodeMyISAM *node, bool compressed) {
    char *prev_key;
    char *curr_key;
    for (int i = 0; i < node->size; i++) {
        MyISAMhead *head = GetHeaderMyISAM(node, i);
        if (compressed || head->pfx_len == 0 || i == 0) {
            curr_key = PageOffset(node, head->key_offset);
            cout << unsigned(head->pfx_len) << ":" << curr_key << ",";
        }
        else {
            curr_key = new char[head->pfx_len + head->key_len + 1];
            strncpy(curr_key, prev_key, head->pfx_len);
            strcpy(curr_key + head->pfx_len, PageOffset(node, head->key_offset));
            cout << curr_key << ",";
        }
        prev_key = curr_key;
    }
}

void printKeys_wt(NodeWT *node, bool compressed) {
    char *prev_key;
    char *curr_key;
    for (int i = 0; i < node->size; i++) {
        WThead *head = GetHeaderWT(node, i);
        if (compressed || head->pfx_len == 0 || i == 0) {
            curr_key = PageOffset(node, head->key_offset);
            cout << unsigned(head->pfx_len) << ":" << curr_key << ",";
        }
        else {
            curr_key = new char[head->pfx_len + head->key_len + 1];
            strncpy(curr_key, prev_key, head->pfx_len);
            strcpy(curr_key + head->pfx_len, PageOffset(node, head->key_offset));
            cout << curr_key << ",";
        }

        prev_key = curr_key;
    }
}

void printKeys_pkb(NodePkB *node, bool compressed) {
    string curr_key;
    for (int i = 0; i < node->size; i++) {
        PkBhead *head = GetHeaderPkB(node, i);
        if (compressed) {
            cout << unsigned(head->pfx_len) << ":" << head->pk << "("
                 << PageOffset(node, head->key_offset) << ")"
                 << ",";
        }
        else {
            cout << PageOffset(node, head->key_offset) << ",";
        }
    }
}