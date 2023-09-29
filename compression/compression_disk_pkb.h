#pragma once
#include <iostream>
#include <vector>
#include "../tree_disk/node_disk.cpp"
#include "../utils/compare.cpp"

using namespace std;

enum cmp {
    EQ = 0,
    GT = 1,
    LT = -1,
};

struct compareKeyResult {
    cmp comp;
    int offset;
};

struct findNodeResult {
    int low;
    int high;
    int offset;
};

void generate_pkb_key(NodePkB *node, char *newkey, int keylen, int insertpos,
                      NodePkB **path, int path_level, Item &key, FILE *fp);

void get_base_key_from_ancestor(NodePkB **path, int path_level, NodePkB *node, Item &key, FILE *fp);

void update_next_prefix(NodePkB *node, int pos, char *full_newkey, int keylen, FILE *fp);

compareKeyResult compare_key(const char *a, const char *b, int alen, int blen, bool fullcomp = true);

compareKeyResult compare_part_key(NodePkB *node, int idx, const char *searchKey, int keylen, cmp comp, int offset);

findNodeResult find_node(NodePkB *node, const char *searchKey, int keylen,
                         int offset, bool &equal, DskManager *dsk);

findNodeResult find_bit_tree(NodePkB *node, const char *searchKey, int keylen,
                             int low, int high, int offset, bool &equal, DskManager *dsk);