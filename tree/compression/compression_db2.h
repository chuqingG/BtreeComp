#pragma once
#include <iostream>
#include "../node.cpp"
#include "../util.cpp"

enum optimizationType {
    prefixmerge,
    prefixexpand
};

struct prefixMergeSegment {
    vector<PrefixMetaData> segment;
    Data prefix;
    int cost = INT32_MAX;
    int firstindex = 0;
};

struct closedRange {
    vector<PrefixMetaData> prefixMetadatas;
    // vector<int> positions;
    int pos_low = 0;
    int pos_high = -1; // To make sure the uninitialized struct high - low < 0
};

struct db2split {
    vector<PrefixMetaData> leftmetadatas;
    vector<PrefixMetaData> rightmetadatas;
    Data *splitprefix;
};

// The offset in prefix_merge is length to add before head,
//            in prefix_expand is the actual key_offset
class prefixOptimization {
public:
    int memusage;
    char *base;
    uint8_t *newsize;
    uint16_t *newoffset;
    vector<PrefixMetaData> prefixes;

    prefixOptimization();
    ~prefixOptimization();
};
// struct prefixOptimization {
//     // change to a class
//     int memusage = 0;
//     char* base = NewPage();
//     uint8_t *newsize = new uint8_t[kNumberBound * DB2_COMP_RATIO];
//     uint16_t *newoffset = new uint16_t[kNumberBound * DB2_COMP_RATIO];
//     vector<PrefixMetaData> prefixes;

//     ~prefixOptimization(){
//         delete base;
//         delete newsize;
//         delete newoffset;
//     }
// };

closedRange find_closed_range(vector<PrefixMetaData> &prefixmetadatas, const char *newprefix, int prefixlen, int p_i_pos);

// Cost function is defined based on space requirements for prefix + suffixes
// Cost must be minimized
int calculate_prefix_merge_cost(prefixOptimization *result, vector<PrefixMetaData> segment, Data *prefix);

// Cost of optimization is calculated as size of prefixes + size of suffixes
// int calculate_cost_of_optimization(prefixOptimization result);

// // Find optimization with minimum cost to apply
// optimizationType find_prefix_optimization_to_apply(prefixOptimization prefixExpandResult,
//                                                    prefixOptimization prefixMergeResult);

prefixMergeSegment find_best_segment_of_size_k(prefixOptimization *result, closedRange *closedRange, int k);

prefixMergeSegment find_best_segment_in_closed_range(prefixOptimization *result, closedRange *closedRange);

void merge_prefixes_in_segment(prefixOptimization *result,
                               prefixMergeSegment *bestsegment);

prefixOptimization *prefix_merge(DB2Node *node);

int expand_prefixes_in_boundary(prefixOptimization *result, int index);

prefixOptimization *prefix_expand(DB2Node *node);

// int search_prefix_metadata(DB2Node *cursor, string_view key);

int find_prefix_pos(DB2Node *cursor, const char *key, int keylen, bool forinsert);

// int find_insert_pos(DB2Node *node, const char* key, int keylen, int (*insertfunc)(DB2Node *, string_view, int, int, bool &), bool &equal);

void apply_prefix_optimization(DB2Node *node);