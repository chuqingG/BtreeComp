#pragma once
#include <iostream>
#include "../node.cpp"
#include "../node_inline.h"
#include "../util.cpp"

struct closedRange {
    int low = 0;
    int high = -1; // To make sure the uninitialized struct high - low < 0
};

struct prefixMergeSegment {
    closedRange segment;
    Item prefix;
    int save = INT32_MIN;
    int firstindex = 0;
};

struct prefixItem {
    Item prefix;
    int low = 0;
    int high = 0;
};

// The offset in prefix_merge is length to add before head,
//            in prefix_expand is the actual key_offset
class prefixOptimization {
public:
    int space_top;
    int pfx_top;
    int pfx_size;
    char *base;

    prefixOptimization();
    ~prefixOptimization();
};
closedRange find_closed_range(prefixOptimization *result,
                              const char *newprefix, int prefixlen, int p_i_pos);

// Cost function is defined based on space requirements for prefix + suffixes
// Cost must be minimized

int calculate_prefix_merge_save(prefixOptimization *result, prefixMergeSegment *seg, Item *prefix);

// Cost of optimization is calculated as size of prefixes + size of suffixes
// int calculate_cost_of_optimization(prefixOptimization result);

prefixMergeSegment find_best_segment_of_size_k(prefixOptimization *result, closedRange *closedRange, int k);

prefixMergeSegment find_best_segment_in_closed_range(prefixOptimization *result, closedRange *closedRange);

void merge_prefixes_in_segment(prefixOptimization *result,
                               prefixMergeSegment *bestsegment);

prefixOptimization *prefix_merge(NodeDB2 *node);

int expand_prefixes_in_boundary(prefixOptimization *result, int index);

prefixOptimization *prefix_expand(NodeDB2 *node);

// int search_prefix_metadata(NodeDB2 *cursor, string_view key);

int find_prefix_pos(NodeDB2 *cursor, const char *key, int keylen, bool forinsert);

// int find_insert_pos(NodeDB2 *node, const char* key, int keylen, int (*insertfunc)(NodeDB2 *, string_view, int, int, bool &), bool &equal);

void apply_prefix_optimization(NodeDB2 *node);