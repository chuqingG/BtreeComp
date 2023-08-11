#pragma once
#include <iostream>
#include <vector>
#include "../node_mt_mc.cpp"

using namespace std;

enum cmp{
    EQ = 0,
    GT = 1,
    LT = -1,
};

struct compareKeyResult{
    cmp comp;
    int offset;
};

struct findNodeResult{
    int low;
    int high;
    int offset;
};

int compute_offset(string prev_key, string key);

string get_key(NodePkB *node, int slot);

KeyPkB generate_pkb_key(NodePkB *node, string newkey, char *newkeyptr, int insertpos, vector<NodePkB *> parents, int parentpos);

void build_page_prefixes(NodePkB *node, int pos);

compareKeyResult compare_key(string a, string b, bool fullcomp = true);

compareKeyResult compare_part_key(string searchKey, KeyPkB indKey, cmp comp, int offset);

findNodeResult find_node(NodePkB *node, string searchKey, int offset);

findNodeResult find_bit_tree(NodePkB *node, string searchKey, int low, int high, int offset);