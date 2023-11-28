#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "node_disk.cpp"
#include "node_disk_inline.h"
#include "../utils/compare.cpp"
#include "../utils/config.h"
#include "dsk_manager.cpp"

using namespace std;

struct keyloc {
    NodeWT *node;
    int off;
    keyloc(NodeWT *n, int idx) {
        node = n;
        off = idx;
    }
};

struct CacheItem {
    CacheItem *prev;
    CacheItem *next;
    int pos;
    WThead *key;
    CacheItem(WThead *k, int p) {
        pos = p;
        key = k;
        prev = nullptr;
        next = nullptr;
    }
};

class Cache {
public:
    char *mem;
    CacheItem *head, *tail;
    unordered_map<WThead *, CacheItem *> mp;
    int size;
    int nextpos = 0;

    Cache() {
        mem = new char[WT_CACHE_KEY_NUM * (APPROX_KEY_SIZE + 1)];
        size = 0;
        head = new CacheItem(nullptr, 0);
        tail = new CacheItem(nullptr, 0);
        head->next = tail;
        tail->prev = head;
        // head = 0;
    }
    ~Cache() {
        while (head) {
            CacheItem *tmp = head;
            head = head->next;
            delete tmp;
        }
        delete[] mem;
    }

    int put(WThead *k) {
        // return the pos
        if (mp.find(k) != mp.end()) {
            CacheItem *item = mp[k];
            // item->pos = pos;
            move_to_head(item);
            return item->pos;
        }
        else {
            CacheItem *newitem = new CacheItem(k, nextpos);
            mp[k] = newitem;
            add_to_head(newitem);
            k->isinitialized = true;
            ++size;
            nextpos++;
            if (size >= WT_CACHE_KEY_NUM) {
                CacheItem *item = delete_tail();
                mp.erase(item->key);
                item->key->isinitialized = false;
                nextpos = item->pos;
                // delete item;
                --size;
            }
            cout << "put: " << k << endl;
            return newitem->pos;
        }
    }

    char *get(WThead *k) {
        cout << "get: " << k << endl;
        if (mp.find(k) != mp.end()) {
            CacheItem *item = mp[k];
            move_to_head(item);
            return mem + item->pos * (APPROX_KEY_SIZE + 1);
        }
        return nullptr;
    }

    void write(int pos, const char *s) {
        char *buf = mem + pos * (APPROX_KEY_SIZE + 1);
        memcpy(buf, s, APPROX_KEY_SIZE);
        buf[APPROX_KEY_SIZE] = '\0';
    }

private:
    void add_to_head(CacheItem *node) {
        head->next->prev = node;
        node->next = head->next;
        node->prev = head;
        head->next = node;
    }

    void move_to_head(CacheItem *node) {
        remove(node);
        add_to_head(node);
    }

    void remove(CacheItem *node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    CacheItem *delete_tail() {
        CacheItem *tmp = tail->prev;
        remove(tmp);
        return tmp;
    }
};

// BP tree
class BPTreeWT {
public:
    BPTreeWT(bool non_leaf_compression, bool suffix_compression = true);
    ~BPTreeWT();
    int search(const char *);
    void insert(char *);
    int searchRange(const char *min, const char *max);
    void getSize(NodeWT *, int &, int &, int &, int &, unsigned long &, int &);
    int getHeight(NodeWT *);
    NodeWT *getRoot();
    void printTree(NodeWT *x, vector<bool> flag, bool compressed = false,
                   int depth = 0, bool isLast = false);
    DskManager *dsk;
    Cache *cache;

private:
    NodeWT *_root;
    bool non_leaf_comp;
    bool suffix_comp;
    int max_level;
    void insert_leaf(NodeWT *leaf, NodeWT **path, int path_level, char *key, int keylen);
    void insert_nonleaf(NodeWT *node, NodeWT **path, int pos, splitReturnWT *childsplit);
    // insert_binary
    int search_insert_pos(NodeWT *cursor, const char *key, int keylen,
                          int low, int high, uint16_t &skiplow, bool &equal);
    // search_binary
    int search_in_leaf(NodeWT *cursor, const char *key, int keylen, int low, int high, uint16_t &skiplow);
    NodeWT *search_leaf_node(NodeWT *root, const char *key, int keylen, uint16_t &skiplow);
    NodeWT *search_leaf_node_for_insert(NodeWT *root, const char *key, int keylen,
                                        NodeWT **path, int &path_level, uint16_t &skiplow);
    bool check_split_condition(NodeWT *node, int keylen);
    int split_point(NodeWT *node);
    splitReturnWT split_nonleaf(NodeWT *node, int pos, splitReturnWT *childsplit);
    splitReturnWT split_leaf(NodeWT *node, char *newkey, int newkey_len);
};