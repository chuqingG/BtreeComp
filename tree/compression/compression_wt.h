#pragma once
#include <iostream>
#include "../node.cpp"
#include "../node_inline.h"
#include "../../utils/compare.cpp"

using namespace std;

#define WT_KEY_PREFIX_PREVIOUS_MINIMUM 10
#define MIN_PREFIX_GAP_TO_UPDATE 5
#define MIN_PREFIX_LEN 1
int prefix_compression_min = 1;

uint8_t compute_prefix_wt(string prev_key, string key, uint8_t prev_pfx);
// string initialize_prefix_compressed_key(NodeWT *node, int ind);
void get_full_key(NodeWT *node, int slot, Item &key);
// Item extract_key(NodeWT *node, int ind, bool non_leaf);
// void build_page_prefixes(NodeWT *node, int pos, string key_at_pos);
// void record_page_prefix_group(NodeWT *node);
void populate_prefix_backward(NodeWT *node, int pos, char *newkey, int keylen);
uint8_t compute_new_prefix_len(const char *pkey, const char *ckey,
                               int pkey_len, int ckey_len, uint8_t ppfx_len);
void suffix_truncate(Item *lastleft, Item *firstright, bool isleaf, Item &result);
// string promote_key(NodeWT *node, string lastleft, string firstright);
// string get_uncompressed_key_before_insert(NodeWT *node, int ind, int insertpos, string newkey, bool equal);