#pragma once
#include <iostream>
#include "../node_mt.cpp"
#include "../util.cpp"

enum optimizationType
{
    prefixmerge,
    prefixexpand
};

struct prefixMergeSegment
{
    vector<PrefixMetaData> segment;
    string prefix;
    int cost;
    int firstindex;
};

struct closedRange
{
    vector<PrefixMetaData> prefixMetadatas;
    vector<int> positions;
};

struct db2split{
    vector<PrefixMetaData> leftmetadatas;
    vector<PrefixMetaData> rightmetadatas;
    string splitprefix;
};

struct prefixOptimization
{
    vector<Key> keys;
    vector<PrefixMetaData> prefixMetadatas;
};


closedRange find_closed_range(vector<PrefixMetaData> prefixmetadatas, string n_p_i, int p_i_pos);

// Cost function is defined based on space requirements for prefix + suffixes
// Cost must be minimized
int calculate_prefix_merge_cost(vector<Key> keys, vector<PrefixMetaData> segment, string prefix);

// Cost of optimization is calculated as size of prefixes + size of suffixes
int calculate_cost_of_optimization(prefixOptimization result);

// Find optimization with minimum cost to apply
optimizationType find_prefix_optimization_to_apply(prefixOptimization prefixExpandResult,
                                                   prefixOptimization prefixMergeResult);

prefixMergeSegment find_best_segment_of_size_k(vector<Key> keys, closedRange closedRange, int k);

prefixMergeSegment find_best_segment_in_closed_range(vector<Key> keys, closedRange closedRange);

void merge_prefixes_in_segment(vector<Key> &keys, vector<PrefixMetaData> &prefixmetadatas,
                               prefixMergeSegment bestsegment, string newprefix);

prefixOptimization prefix_merge(vector<Key> keys, vector<PrefixMetaData> prefixmetadatas);

int expand_prefixes_in_boundary(vector<Key> &keys, vector<PrefixMetaData> &prefixmetadatas, int index);

prefixOptimization prefix_expand(vector<Key> keys, vector<PrefixMetaData> prefixmetadatas);

db2split peform_split(DB2Node *node, int split, bool isleaf);

int insert_prefix_metadata(DB2Node *cursor, string_view key);

int search_prefix_metadata(DB2Node *cursor, string_view key);

int find_insert_pos(DB2Node *node, string &key, int (*insertfunc)(Node *, string_view, int, int), bool &equal);

void apply_prefix_optimization(DB2Node *node);