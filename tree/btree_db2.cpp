#include "btree_db2.h"

PrefixMetaData::PrefixMetaData() {
    prefix = "";
    low = 0;
    high = 0;
}

PrefixMetaData::PrefixMetaData(string p, int l, int h) {
    prefix = p;
    low = l;
    high = h;
}

// Initialise the BPTreeDB2 DB2Node
BPTreeDB2::BPTreeDB2() {
    _root = NULL;
}

// Destructor of BPTreeDB2 tree
BPTreeDB2::~BPTreeDB2() {
    delete _root;
}

// Function to get the root DB2Node
DB2Node *BPTreeDB2::getRoot() {
    return _root;
}

// Function to find any element
// in B+ Tree
int BPTreeDB2::search(const char *key) {
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(_root, key, parents);
    if (leaf == nullptr)
        return -1;
    int metadatapos = search_prefix_metadata(leaf, key);
    if (metadatapos == -1)
        return -1;
    PrefixMetaData currentMetadata = leaf->prefixMetadata.at(metadatapos);
    string prefix = currentMetadata.prefix;
    // string_view compressed_key = key.substr(prefix.length());
    return search_binary(leaf, key + prefix.length(), currentMetadata.low, currentMetadata.high);
}

void BPTreeDB2::insert(char *x) {
    if (_root == nullptr) {
        _root = new DB2Node;
        //#ifdef DUPKEY
        int rid = rand();
        _root->keys.push_back(Key_c(x, rid));
        _root->size = 1;
        //#else
        //#endif
        _root->IS_LEAF = true;
        PrefixMetaData metadata = PrefixMetaData("", 0, 0);
        _root->prefixMetadata.push_back(metadata);
        return;
    }
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(_root, x, parents);
    insert_leaf(leaf, parents, x);
}

#ifdef TOFIX
// Function to peform range query on B+Tree of DB2
int BPTreeDB2::searchRange(string min, string max) {
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(_root, min, parents);
    int pos, metadatapos;
    int entries = 0;
    if (leaf == nullptr) {
        pos = -1;
    }
    else {
        metadatapos = search_prefix_metadata(leaf, min);
        if (metadatapos == -1) {
            pos = -1;
        }
        else {
            PrefixMetaData currentMetadata = leaf->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            string compressed_key = min.substr(prefix.length());
            pos = search_binary(leaf, compressed_key, currentMetadata.low, currentMetadata.high);
        }
    }
    // Keep searching till value > max or we reach end of tree
    while (pos != -1 && leaf != nullptr) {
        if (pos == leaf->size) {
            pos = 0;
            metadatapos = 0;
            leaf = leaf->next;
            if (leaf == nullptr) {
                break;
            }
        }

        if (pos > leaf->prefixMetadata.at(metadatapos).high) {
            metadatapos += 1;
        }
        string leafkey = leaf->prefixMetadata.at(metadatapos).prefix + leaf->keys.at(pos).value;
        if (lex_compare(leafkey, max) > 0) {
            break;
        }
        entries += leaf->keys.at(pos).ridList.size();
        pos++;
    }
    return entries;
}

// Function to peform backward scan on B+tree
// Backward scan first decompresses the node in the forward direction
// and then scans in the backward direction. This is done to avoid overhead
// of decompression in the backward direction.
int BPTreeDB2::backwardScan(string min, string max) {
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(_root, max, parents);
    int pos, metadatapos;
    int entries = 0;
    if (leaf == nullptr) {
        pos = -1;
    }
    else {
        metadatapos = search_prefix_metadata(leaf, max);
        if (metadatapos == -1) {
            pos = -1;
        }
        else {
            PrefixMetaData currentMetadata = leaf->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            string compressed_key = max.substr(prefix.length());
            pos = search_binary(leaf, compressed_key, currentMetadata.low, currentMetadata.high);
        }
    }
    vector<string> decompressed_keys = decompress_keys(leaf, pos);
    // Keep searching till value > min or we reach end of tree
    while (leaf != nullptr) {
        if (decompressed_keys.size() == 0) {
            leaf = leaf->prev;
            if (leaf == nullptr) {
                break;
            }
            decompressed_keys = decompress_keys(leaf);
            pos = decompressed_keys.size() - 1;
        }
        string leafkey = decompressed_keys.back();
        if (lex_compare(leafkey, min) < 0) {
            break;
        }
        entries += leaf->keys.at(pos).ridList.size();
        decompressed_keys.pop_back();
        pos--;
    }
    return entries;
}

// Decompress key in forward direction until a certain provided index
vector<string> BPTreeDB2::decompress_keys(DB2Node *node, int pos) {
    vector<string> decompressed_keys;
    if (node == nullptr)
        return decompressed_keys;
    int size = pos == -1 ? node->keys.size() - 1 : pos;
    int ind;
    for (int i = 0; i < node->prefixMetadata.size(); i++) {
        for (ind = node->prefixMetadata.at(i).low; ind <= min(node->prefixMetadata.at(i).high, size); ind++) {
            string key = node->prefixMetadata.at(i).prefix + node->keys.at(ind).value;
            decompressed_keys.push_back(key);
        }
        if (ind > size)
            break;
    }
    return decompressed_keys;
}
#endif

void BPTreeDB2::insert_nonleaf(DB2Node *node, vector<DB2Node *> &parents, int parentlevel, splitReturnDB2 childsplit) {
    if (check_split_condition(node)) {
        apply_prefix_optimization(node);
    }
    if (check_split_condition(node)) {
        DB2Node *parent = nullptr;
        if (parentlevel >= 0) {
            parent = parents.at(parentlevel);
        }
        splitReturnDB2 currsplit = split_nonleaf(node, parents, parentlevel, childsplit);
        if (node == _root) {
            DB2Node *newRoot = new DB2Node;
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(currsplit.promotekey), rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            PrefixMetaData metadata = PrefixMetaData("", 0, 0);
            newRoot->prefixMetadata.push_back(metadata);
            _root = newRoot;
        }
        else {
            if (parent == nullptr)
                return;
            insert_nonleaf(parent, parents, parentlevel - 1, currsplit);
        }
    }
    else {
        string promotekey = childsplit.promotekey;
        bool equal = false;
        int insertpos = find_insert_pos(node, promotekey.data(), this->insert_binary, equal);
        int rid = rand();
        vector<Key_c> allkeys;
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }
        allkeys.push_back(Key_c(string_to_char(promotekey), rid));
        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }

        vector<DB2Node *> allptrs;
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
    }
}

void BPTreeDB2::insert_leaf(DB2Node *leaf, vector<DB2Node *> &parents, char *key) {
    if (check_split_condition(leaf)) {
        apply_prefix_optimization(leaf);
    }
    if (check_split_condition(leaf)) {
        splitReturnDB2 split = split_leaf(leaf, parents, key);
        if (leaf == _root) {
            DB2Node *newRoot = new DB2Node;
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(split.promotekey), rid));
            newRoot->ptrs.push_back(split.left);
            newRoot->ptrs.push_back(split.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            PrefixMetaData metadata = PrefixMetaData("", 0, 0);
            newRoot->prefixMetadata.push_back(metadata);
            _root = newRoot;
        }
        else {
            DB2Node *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }
    else {
        int insertpos;
        int rid = rand();
        bool equal = false;
        insertpos = find_insert_pos(leaf, key, this->insert_binary, equal);
        vector<Key_c> allkeys;
        if (equal) {
            allkeys = leaf->keys;
            allkeys.at(insertpos - 1).addRecord(rid);
        }
        else {
            for (int i = 0; i < insertpos; i++) {
                allkeys.push_back(leaf->keys.at(i));
            }
            allkeys.push_back(Key_c(string_to_char(key), rid));
            for (int i = insertpos; i < leaf->size; i++) {
                allkeys.push_back(leaf->keys.at(i));
            }
        }
        leaf->keys = allkeys;
        if (!equal) {
            leaf->size = leaf->size + 1;
        }
    }
}

int BPTreeDB2::split_point(vector<Key_c> allkeys) {
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnDB2 BPTreeDB2::split_nonleaf(DB2Node *node, vector<DB2Node *> parents, int pos, splitReturnDB2 childsplit) {
    splitReturnDB2 newsplit;
    DB2Node *right = new DB2Node;
    string promotekey = childsplit.promotekey;
    int insertpos;
    int rid = rand();
    bool equal = false;
    insertpos = find_insert_pos(node, promotekey.data(), this->insert_binary, equal);
    vector<Key_c> allkeys;
    vector<DB2Node *> allptrs;

    for (int i = 0; i < insertpos; i++) {
        allkeys.push_back(node->keys.at(i));
    }
    // Non-leaf nodes won't have duplicates
    allkeys.push_back(Key_c(string_to_char(promotekey), rid));
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

    int split = split_point(allkeys);

    // cout << "Best Split point " << split << " size " << allkeys.size() << endl;

    vector<Key_c> leftkeys;
    vector<Key_c> rightkeys;
    vector<DB2Node *> leftptrs;
    vector<DB2Node *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    db2split splitresult = perform_split(node, split, false);
    node->prefixMetadata = splitresult.leftmetadatas;
    right->prefixMetadata = splitresult.rightmetadatas;
    newsplit.promotekey = splitresult.splitprefix + allkeys.at(split).value;

    node->size = leftkeys.size();
    node->keys = leftkeys;
    node->ptrs = leftptrs;

    right->size = rightkeys.size();
    right->IS_LEAF = false;
    right->keys = rightkeys;
    right->ptrs = rightptrs;

    // Set prev and next pointers
    DB2Node *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnDB2 BPTreeDB2::split_leaf(DB2Node *node, vector<DB2Node *> &parents, char *newkey) {
    splitReturnDB2 newsplit;
    DB2Node *right = new DB2Node;
    int insertpos;
    int rid = rand();
    bool equal = false;
    insertpos = find_insert_pos(node, newkey, this->insert_binary, equal);

    vector<Key_c> allkeys;

    if (equal) {
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }
        allkeys.push_back(Key_c(newkey, rid));
        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }
    }

    int split = split_point(allkeys);

    vector<Key_c> leftkeys;
    vector<Key_c> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));

    db2split splitresult = perform_split(node, split, true);
    node->prefixMetadata = splitresult.leftmetadatas;
    right->prefixMetadata = splitresult.rightmetadatas;

    node->size = leftkeys.size();
    node->keys = leftkeys;

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;

    // Set prev and next pointers
    DB2Node *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    string firstright = right->keys.at(0).value;
    PrefixMetaData firstrightmetadata = right->prefixMetadata.at(0);
    firstright = firstrightmetadata.prefix + firstright;
    newsplit.promotekey = firstright;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeDB2::check_split_condition(DB2Node *node) {
    int currspace = 0;
#ifdef SPLIT_STRATEGY_SPACE
    for (uint32_t i = 0; i < node->prefixMetadata.size(); i++) {
        currspace += node->prefixMetadata.at(i).prefix.length();
    }
    for (uint32_t i = 0; i < node->keys.size(); i++) {
        currspace += node->keys.at(i).getSize();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeDB2::insert_binary(DB2Node *cursor, const char *key, int low, int high, bool &equal) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
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

DB2Node *BPTreeDB2::search_leaf_node(DB2Node *searchroot, const char *key, vector<DB2Node *> &parents) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    DB2Node *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        string_view searchkey = key;
        parents.push_back(cursor);
        int pos = -1;
        int metadatapos = insert_prefix_metadata(cursor, key);
        if (metadatapos == cursor->prefixMetadata.size()) {
            pos = cursor->size;
        }
        else {
            PrefixMetaData currentMetadata = cursor->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            // int prefixcmp = key.compare(0, prefix.length(), prefix);
            int prefixcmp = strncmp(key, prefix.data(), prefix.length());
            if (prefixcmp != 0) {
                pos = prefixcmp > 0 ? currentMetadata.high + 1 : currentMetadata.low;
            }
            else {
                // searchkey = key.substr(prefix.length());
                pos = insert_binary(cursor, key + prefix.length(), currentMetadata.low, currentMetadata.high, equal);
            }
        }
        cursor = cursor->ptrs.at(pos);
    }
    return cursor;
}

int BPTreeDB2::search_binary(DB2Node *cursor, const char *key, int low, int high) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    // cout << "key:" << key << endl;
    // cout << "node: ";
    // for (auto key : cursor->keys)
    //     cout << key.value << ",";
    // cout << endl;
    return -1;
}

/*
================================================
=============statistic function & printer=======
================================================
*/

void BPTreeDB2::getSize(DB2Node *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                        int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
        int prefixSize = 0;
        for (uint32_t i = 0; i < cursor->prefixMetadata.size(); i++) {
            currSize += cursor->prefixMetadata.at(i).prefix.length();
            prefixSize += cursor->prefixMetadata.at(i).prefix.length();
        }
        for (int i = 0; i < cursor->size; i++) {
            currSize += cursor->keys.at(i).getSize();
        }
        totalKeySize += currSize;
        numKeys += cursor->size;
        totalPrefixSize += prefixSize;
        numNodes += 1;
        if (cursor->IS_LEAF != true) {
            totalBranching += cursor->ptrs.size();
            numNonLeaf += 1;
            for (int i = 0; i < cursor->size + 1; i++) {
                getSize(cursor->ptrs[i], numNodes, numNonLeaf, numKeys, totalBranching, totalKeySize, totalPrefixSize);
            }
        }
    }
}

int BPTreeDB2::getHeight(DB2Node *cursor) {
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

void BPTreeDB2::printTree(DB2Node *x, vector<bool> flag, bool compressed, int depth, bool isLast) {
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
        printKeys_db2(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast) {
        cout << "+--- ";
        printKeys_db2(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }
    else {
        cout << "+--- ";
        printKeys_db2(x, compressed);
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