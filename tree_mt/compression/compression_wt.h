#pragma once
#include <iostream>
#include "../node.h"
#include "../node_mt.cpp"

using namespace std;

#define WT_KEY_PREFIX_PREVIOUS_MINIMUM 10
int prefix_compression_min = 1;

uint8_t compute_prefix_wt(string prev_key, string key);
uint8_t compute_prefix_wt(string prev_key, string key, uint8_t prev_pfx);
string initialize_prefix_compressed_key(NodeWT *node, int ind);
void get_full_key(NodeWT *node, int slot, WTitem &key);
string extract_key(NodeWT *node, int ind, bool non_leaf);
void build_page_prefixes(NodeWT *node, int pos, string key_at_pos);
void record_page_prefix_group(NodeWT *node);
string promote_key(NodeWT *node, string lastleft, string firstright);
string get_uncompressed_key_before_insert(NodeWT *node, int ind, int insertpos, string newkey, bool equal);
// void suffix_truncate(WTitem *lastleft, WTitem *firstright, bool isleaf, WTitem &result);