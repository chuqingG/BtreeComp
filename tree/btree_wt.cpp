#include "btree_wt.h"
#include "./compression/compression_wt.cpp"

// Initialise the BPTreeWT NodeWT
BPTreeWT::BPTreeWT(bool non_leaf_compression,
                   bool suffix_compression) {
    _root = NULL;
    max_level = 1;
    non_leaf_comp = non_leaf_compression;
    suffix_comp = suffix_compression;
}

// Destructor of BPTreeWT tree
BPTreeWT::~BPTreeWT() {
    delete _root;
}

// Function to get the root NodeWT
NodeWT *BPTreeWT::getRoot() {
    return _root;
}

// Function to find any element
// in B+ Tree
int BPTreeWT::search(const char *key) {
    int keylen = strlen(key);
    uint8_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(_root, key, keylen, skiplow);
    if (leaf == nullptr)
        return -1;
    return search_binary(leaf, key, 0, leaf->size - 1, skiplow);
}

void BPTreeWT::insert(char *x) {
    int keylen = strlen(x);
    if (_root == nullptr) {
        _root = new NodeWT();
        // int rid = rand();
        // _root->keys.push_back(KeyWT(x, 0, rid));
        // _root->size = 1;
        InsertKeyWT(_root, 0, x, keylen, 0);
        _root->IS_LEAF = true;
        return;
    }

    NodeWT *search_path[max_level];
    int path_level = 0;
    uint8_t skiplow = 0;
    NodeWT *leaf = search_leaf_node_for_insert(_root, x, keylen, search_path, path_level, skiplow);
    insert_leaf(leaf, search_path, path_level, x, keylen);
}

void BPTreeWT::insert_nonleaf(NodeWT *node, vector<NodeWT *> &parents, int pos, splitReturnWT childsplit) {
    if (check_split_condition(node)) {
        NodeWT *parent = nullptr;
        if (pos >= 0) {
            parent = parents.at(pos);
        }
        splitReturnWT currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == _root) {
            NodeWT *newRoot = new NodeWT;
            int rid = rand();
            newRoot->keys.push_back(KeyWT(currsplit.promotekey, 0, rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            _root = newRoot;
        }
        else {
            if (parent == nullptr)
                return;
            insert_nonleaf(parent, parents, pos - 1, currsplit);
        }
    }
    else {
        string promotekey = childsplit.promotekey;
        uint8_t skiplow = 0;
        int rid = rand();
        bool equal = false;
        int insertpos = insert_binary(node, promotekey, 0, node->size - 1, skiplow, equal);
        vector<KeyWT> allkeys;
        vector<NodeWT *> allptrs;

        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }

        if (this->non_leaf_comp) {
            if (insertpos > 0) {
                string prev_key = get_key(node, insertpos - 1);
                uint8_t pfx = compute_prefix_wt(prev_key, promotekey);
                string compressed = promotekey.substr(pfx);
                allkeys.push_back(KeyWT(compressed, pfx, rid));
            }
            else {
                allkeys.push_back(KeyWT(promotekey, 0, rid));
            }
        }
        else {
            allkeys.push_back(KeyWT(promotekey, 0, rid));
        }

        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }

        for (int i = 0; i < insertpos + 1; i++) {
            allptrs.push_back(node->ptrs.at(i));
        }
        allptrs.push_back(childsplit.right);
        for (int i = insertpos + 1; i < node->size + 1; i++) {
            allptrs.push_back(node->ptrs.at(i));
        }
        node->keys = allkeys;
        node->ptrs = allptrs;
        node->size = node->size + 1;

        if (this->non_leaf_comp) {
            // Rebuild prefixes if a new item is inserted in position 0
            build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
        }
    }
}

void BPTreeWT::insert_leaf(NodeWT *leaf, NodeWT **path, int path_level, char *key, int keylen) {
    if (check_split_condition(leaf, keylen)) {
        splitReturnWT split = split_leaf(leaf, parents, key);
        if (leaf == _root) {
            NodeWT *newRoot = new NodeWT();

            // newRoot->keys.push_back(KeyWT(split.promotekey, 0, rid));
            InsertKeyWT(newRoot, 0,
                        split.promotekey.addr, split.promotekey.size, 0);
            newRoot->ptrs.push_back(split.left);
            newRoot->ptrs.push_back(split.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
        }
        else {
            NodeWT *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }
    else {
        uint8_t skiplow = 0;
        bool equal = false;
        int insertpos = search_insert_pos(leaf, key, keylen,
                                          0, leaf->size - 1, skiplow, equal);

        if (insertpos == 0) {
            InsertKeyWT(leaf, insertpos, key, keylen, 0);
        }
        else {
            // fetch the previous key, compute prefix_len
            WTitem prevkey = get_key(leaf, insertpos - 1);
            uint8_t pfx_len = get_common_prefix_len(prevkey.addr, key,
                                                    prevkey.size, keylen);
            InsertKeyWT(leaf, insertpos, key + pfx_len, keylen - pfx_len, pfx_len);
        }
        // vector<KeyWT> allkeys;
        //
        // for (int i = 0; i < insertpos; i++) {
        //     allkeys.push_back(leaf->keys.at(i));
        // }
        // if (insertpos > 0) {
        //     string prev_key = get_key(leaf, insertpos - 1);
        //     uint8_t pfx = compute_prefix_wt(prev_key, key);
        //     string compressed = key.substr(pfx);
        //     allkeys.push_back(KeyWT(compressed, pfx, rid));
        // }
        // else {
        //     allkeys.push_back(KeyWT(key, 0, rid));
        // }
        // for (int i = insertpos; i < leaf->size; i++) {
        //     allkeys.push_back(leaf->keys.at(i));
        // }
        // leaf->keys = allkeys;

        if (!equal) {
            // leaf->size = leaf->size + 1;
            build_page_prefixes(leaf, insertpos, key, keylen); // Populate new prefixes
        }
    }
}

int BPTreeWT::split_point(vector<KeyWT> allkeys) {
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnWT BPTreeWT::split_nonleaf(NodeWT *node, vector<NodeWT *> parents, int pos, splitReturnWT childsplit) {
    splitReturnWT newsplit;
    NodeWT *right = new NodeWT;
    string promotekey = childsplit.promotekey;
    string splitprefix;
    uint8_t skiplow = 0;
    int rid = rand();
    bool equal = false;
    int insertpos = insert_binary(node, promotekey, 0, node->size - 1, skiplow, equal);

    vector<KeyWT> allkeys;
    vector<NodeWT *> allptrs;

    for (int i = 0; i < insertpos; i++) {
        allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
    }

    // Non-leaf nodes won't have duplicates
    if (this->non_leaf_comp) {
        if (insertpos > 0) {
            string prev_key = get_key(node, insertpos - 1);
            uint8_t pfx = compute_prefix_wt(prev_key, promotekey);
            string compressed = promotekey.substr(pfx);
            allkeys.push_back(KeyWT(compressed, pfx, rid));
        }
        else {
            allkeys.push_back(KeyWT(promotekey, 0, rid));
        }
    }
    else {
        allkeys.push_back(KeyWT(promotekey, 0, rid));
    }

    for (int i = insertpos; i < node->size; i++) {
        allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
    }

    for (int i = 0; i < insertpos + 1; i++) {
        allptrs.push_back(node->ptrs.at(i));
    }
    allptrs.push_back(childsplit.right);
    for (int i = insertpos + 1; i < node->size + 1; i++) {
        allptrs.push_back(node->ptrs.at(i));
    }

    int split = split_point(allkeys);

    // cout << "Best Split point " << split << " size " << allkeys.size() << endl;

    vector<KeyWT> leftkeys;
    vector<KeyWT> rightkeys;
    vector<NodeWT *> leftptrs;
    vector<NodeWT *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    string splitkey;
    string firstright;

    if (this->non_leaf_comp) {
        firstright = get_uncompressed_key_before_insert(node, split + 1, insertpos, promotekey, equal);
        rightkeys.at(0) = KeyWT(firstright, 0, rightkeys.at(0).ridList);
        splitkey = get_uncompressed_key_before_insert(node, split, insertpos, promotekey, equal);
    }
    else {
        splitkey = allkeys.at(split).value;
    }

    newsplit.promotekey = splitkey;

    node->size = leftkeys.size();
    node->keys = leftkeys;
    node->ptrs = leftptrs;

    right->size = rightkeys.size();
    right->IS_LEAF = false;
    right->keys = rightkeys;
    right->ptrs = rightptrs;

    // Set next pointers
    NodeWT *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    if (this->non_leaf_comp) {
        if (insertpos < split) {
            build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
        }
        build_page_prefixes(right, 0, firstright); // Populate new prefixes
    }

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnWT BPTreeWT::split_leaf(NodeWT *node, vector<NodeWT *> &parents, string newkey) {
    splitReturnWT newsplit;
    NodeWT *right = new NodeWT;
    uint8_t skiplow = 0;
    int rid = rand();
    bool equal = false;
    int insertpos = insert_binary(node, newkey, 0, node->size - 1, skiplow, equal);
    vector<KeyWT> allkeys;

    if (equal) {
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
        }
        if (insertpos > 0) {
            string prev_key = get_key(node, insertpos - 1);
            uint8_t pfx = compute_prefix_wt(prev_key, newkey);
            string compressed = newkey.substr(pfx);
            allkeys.push_back(KeyWT(compressed, pfx, rid));
        }
        else {
            allkeys.push_back(KeyWT(newkey, 0, rid));
        }

        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
        }
    }

    int split = split_point(allkeys);

    vector<KeyWT> leftkeys;
    vector<KeyWT> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));
    int orgind;
    string firstright;

    firstright = get_uncompressed_key_before_insert(node, split, insertpos, newkey, equal);
    rightkeys.at(0) = KeyWT(firstright, 0, rightkeys.at(0).ridList);

    if (this->suffix_comp) {
        string lastleft = get_uncompressed_key_before_insert(node, split - 1, insertpos, newkey, equal);
        newsplit.promotekey = promote_key(node, lastleft, firstright);
    }
    else {
        newsplit.promotekey = firstright;
    }

    node->size = leftkeys.size();
    node->keys = leftkeys;

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;

    if (!equal && insertpos < split) {
        build_page_prefixes(node, insertpos, newkey); // Populate new prefixes
    }
    build_page_prefixes(right, 0, firstright); // Populate new prefixes

    // Set next pointers
    NodeWT *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeWT::check_split_condition(NodeWT *node, int keylen) {
    // int currspace = 0;
    // for (int i = 0; i < node->keys.size(); i++) {
    //     currspace += node->keys.at(i).getSize();
    // }
    // return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
    if (node->space_top + node->size * sizeof(WThead) + 2 * keylen
        >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    else
        return false;
}

int BPTreeWT::search_insert_pos(NodeWT *cursor, const char *key, int keylen,
                                int low, int high, uint8_t &skiplow, bool &equal) {
    uint8_t skiphigh = 0;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        WTitem curkey;
        if (!cursor->IS_LEAF && this->non_leaf_comp) {
            // non-compressed
            WThead *header = GetHeader(cursor, mid);
            curkey.addr = PageOffset(cursor, header->key_offset);
            curkey.size = header->key_len;
        }
        else {
            curkey = get_key(cursor, mid);
        }
        // char *pagekey = extract_key(cursor, mid, this->non_leaf_comp);
        int cmp;
        uint8_t match = min(skiplow, skiphigh);
        cmp = char_cmp_skip(key, curkey.addr, keylen, curkey.size, &match);
        // cmp = lex_compare_skip(string(key), curkey, &match);
        if (cmp > 0) {
            skiplow = match;
        }
        else if (cmp < 0) {
            skiphigh = match;
        }
        if (cmp == 0) {
            equal = true;
            return mid + 1;
        }
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return high + 1;
}

NodeWT *BPTreeWT::search_leaf_node(NodeWT *searchroot, const char *key, int keylen, uint8_t &skiplow) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeWT *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        // string_view searchkey = key;
        int pos = search_insert_pos(cursor, key, keylen, 0, cursor->size - 1, skiplow, equal);
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

NodeWT *BPTreeWT::search_leaf_node_for_insert(NodeWT *searchroot, const char *key, int keylen,
                                              NodeWT **path, int &path_level, uint8_t &skiplow) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeWT *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        // string_view searchkey = key;
        // parents.push_back(cursor);
        path[path_level++] = cursor;
        int pos = search_insert_pos(cursor, key, keylen, 0, cursor->size - 1, skiplow, equal);
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

int BPTreeWT::search_binary(NodeWT *cursor, string_view key, int low, int high, uint8_t &skiplow) {
    skiphigh = 0;
    match;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        string pagekey = extract_key(cursor, mid, this->non_leaf_comp);
        int cmp;
        match = min(skiplow, skiphigh);
        cmp = lex_compare_skip(string(key), pagekey, &match);
        if (cmp > 0) {
            skiplow = match;
        }
        else if (cmp < 0) {
            skiphigh = match;
        }
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

/*
================================================
=============statistic function & printer=======
================================================
*/

void BPTreeWT::getSize(NodeWT *cursor, int &numNodeWTs, int &numNonLeaf, int &numKeyWTs,
                       int &totalBranching, unsigned long &totalKeyWTSize, int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
        int prefixSize = 0;
        for (int i = 0; i < cursor->size; i++) {
            currSize += cursor->keys.at(i).getSize();
            prefixSize += sizeof(cursor->keys.at(i).prefix);
        }
        totalKeyWTSize += currSize;
        numKeyWTs += cursor->size;
        totalPrefixSize += prefixSize;
        numNodeWTs += 1;
        if (cursor->IS_LEAF != true) {
            totalBranching += cursor->ptrs.size();
            numNonLeaf += 1;
            for (int i = 0; i < cursor->size + 1; i++) {
                getSize(cursor->ptrs[i], numNodeWTs, numNonLeaf, numKeyWTs, totalBranching, totalKeyWTSize, totalPrefixSize);
            }
        }
    }
}

int BPTreeWT::getHeight(NodeWT *cursor) {
    if (cursor == NULL)
        return 0;
    if (cursor->IS_LEAF == true)
        return 1;
    int maxHeight = 0;
    for (int i = 0; i < cursor->size + 1; i++) {
        maxHeight = max(maxHeight, getHeight(cursor->ptrs.at(i)));
    }
    return maxHeight + 1;
}

void BPTreeWT::printTree(NodeWT *x, vector<bool> flag, bool compressed, int depth, bool isLast) {
    // Condition when node is None
    if (x == NULL)
        return;

    // Loop to print the depths of the
    // current node
    for (int i = 1; i < depth; ++i) {
        // Condition when the depth
        // is exploring
        if (flag[i] == true) {
            cout << "| "
                 << " "
                 << " "
                 << " ";
        }

        // Otherwise print
        // the blank spaces
        else {
            cout << " "
                 << " "
                 << " "
                 << " ";
        }
    }

    // Condition when the current
    // node is the root node
    if (depth == 0) {
        printKeys_wt(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast) {
        cout << "+--- ";
        printKeys_wt(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }
    else {
        cout << "+--- ";
        printKeys_wt(x, compressed);
        cout << endl;
    }

    int it = 0;
    for (auto i = x->ptrs.begin();
         i != x->ptrs.end(); ++i, ++it)

        // Recursive call for the
        // children nodes
        printTree(*i, flag, compressed, depth + 1,
                  it == (x->ptrs.size()) - 1);
    flag[depth] = true;
}