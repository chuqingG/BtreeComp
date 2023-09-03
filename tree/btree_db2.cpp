#include "btree_db2.h"

// Initialise the BPTreeDB2 DB2Node
BPTreeDB2::BPTreeDB2() {
    _root = NULL;
    max_level = 1;
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
    int keylen = strlen(key);
    DB2Node *leaf = search_leaf_node(_root, key, keylen);
    if (leaf == nullptr)
        return -1;
    int metadatapos = find_prefix_pos(leaf, key, keylen, false);
    if (metadatapos == -1)
        return -1;
    PrefixMetaData pfx = leaf->prefixMetadata[metadatapos];
    int prefixlen = pfx.prefix->size;
    // string_view compressed_key = key.substr(prefix.length());
    return search_in_leaf(leaf, key + prefixlen, keylen - prefixlen, pfx.low, pfx.high);
}

void BPTreeDB2::insert(char *x) {
    int keylen = strlen(x);
    if (_root == nullptr) {
        _root = new DB2Node;
#ifdef DUPKEY
        int rid = rand();
        _root->keys.push_back(Key_c(x, rid));
        _root->size = 1;
#else
        InsertKey(_root, 0, x, keylen);
#endif
        _root->IS_LEAF = true;
        // PrefixMetaData metadata = PrefixMetaData("", 0, 0, 0);
        // _root->prefixMetadata.push_back(metadata);
        return;
    }
    DB2Node *search_path[max_level];
    int path_level = 0;
    DB2Node *leaf = search_leaf_node_for_insert(_root, x, keylen, search_path, path_level);
    insert_leaf(leaf, search_path, path_level, x, keylen);
}

void BPTreeDB2::insert_nonleaf(DB2Node *node, DB2Node **path,
                               int parentlevel, splitReturnDB2 childsplit) {
    if (check_split_condition(node, childsplit.promotekey->size)) {
        apply_prefix_optimization(node);
    }
    if (check_split_condition(node, childsplit.promotekey->size)) {
        DB2Node *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturnDB2 currsplit = split_nonleaf(node, parentlevel, childsplit);
        if (node == _root) {
            DB2Node *newRoot = new DB2Node;

            InsertNode(newRoot, 0, currsplit.left);
            InsertKey(newRoot, 0, currsplit.promotekey->addr(), currsplit.promotekey->size);
            InsertNode(newRoot, 1, currsplit.right);

            newRoot->IS_LEAF = false;
            // newRoot->size = 1;
            // PrefixMetaData metadata = PrefixMetaData("", 0, 0);
            // newRoot->prefixMetadata.push_back(metadata);
            _root = newRoot;
            max_level++;
        }
        else {
            if (parent == nullptr)
                return;
            insert_nonleaf(parent, path, parentlevel - 1, currsplit);
        }
    }
    else {
        // string promotekey = childsplit.promotekey;
        bool equal = false;
        int insertpos = insert_prefix_and_key(node,
                                              childsplit.promotekey->addr(),
                                              childsplit.promotekey->size, equal);

        // int rid = rand();
        // vector<Key_c> allkeys;
        // for (int i = 0; i < insertpos; i++) {
        //     allkeys.push_back(node->keys.at(i));
        // }
        // allkeys.push_back(Key_c(string_to_char(promotekey), rid));
        // for (int i = insertpos; i < node->size; i++) {
        //     allkeys.push_back(node->keys.at(i));

        // vector<DB2Node *> allptrs;
        // for (int i = 0; i < insertpos + 1; i++) {
        //     allptrs.push_back(node->ptrs.at(i));
        // }
        // allptrs.push_back(childsplit.right);
        // for (int i = insertpos + 1; i < node->size + 1; i++) {
        //     allptrs.push_back(node->ptrs.at(i));
        // }

        InsertNode(node, insertpos + 1, childsplit.right);

        // node->keys = allkeys;
        // node->ptrs = allptrs;
        // node->size = node->size + 1;
    }
}

void BPTreeDB2::insert_leaf(DB2Node *leaf, DB2Node **path, int path_level, char *key, int keylen) {
    if (check_split_condition(leaf, keylen)) {
        apply_prefix_optimization(leaf);
    }
    if (check_split_condition(leaf, keylen)) {
        splitReturnDB2 split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            DB2Node *newRoot = new DB2Node;
            InsertNode(newRoot, 0, split.left);
            InsertKey(newRoot, 0, split.promotekey->addr(), split.promotekey->size);
            InsertNode(newRoot, 1, split.right);

            // newRoot->keys.push_back(Key_c(string_to_char(split.promotekey), rid));
            // newRoot->ptrs.push_back(split.left);
            // newRoot->ptrs.push_back(split.right);
            // PrefixMetaData metadata = PrefixMetaData("", 0, 0, 0);
            // newRoot->prefixMetadata.push_back(metadata);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            DB2Node *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, split);
        }
    }
    else {
        int insertpos;
        bool equal = false;
        // The key was concated inside it
        insertpos = insert_prefix_and_key(leaf, key, keylen, equal);
#ifndef DUPKEY
        // Nothing needed
#else
        vector<Key_c> allkeys;
        int rid = rand();
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
#endif
    }
}

int BPTreeDB2::insert_prefix_and_key(DB2Node *node, const char *key, int keylen, bool &equal) {
    // Find a pos, then insert into it
    int pfx_pos = find_prefix_pos(node, key, keylen, true);
    int insertpos;
    if (pfx_pos == node->prefixMetadata.size()) {
        // need to add new prefix
        insertpos = node->size;
        PrefixMetaData newmetadata = PrefixMetaData("", 0, insertpos, insertpos);
        node->prefixMetadata.insert(node->prefixMetadata.begin() + pfx_pos, newmetadata);
    }
    else {
        PrefixMetaData metadata = node->prefixMetadata[pfx_pos];
        auto prefix = metadata.prefix;
        if (strncmp(key, prefix->addr(), prefix->size) == 0) {
            insertpos = search_insert_pos(node, key + prefix->size, keylen - prefix->size,
                                          metadata.low, metadata.high, equal);
            if (!equal)
                node->prefixMetadata[pfx_pos].high += 1;
        }
        else {
            insertpos = node->prefixMetadata.at(pfx_pos).low;
            PrefixMetaData newmetadata = PrefixMetaData("", 0, insertpos, insertpos);
            node->prefixMetadata.insert(node->prefixMetadata.begin() + pfx_pos, newmetadata);
        }
        if (!equal) {
            for (uint32_t i = pfx_pos + 1; i < node->prefixMetadata.size(); i++) {
                node->prefixMetadata.at(i).low += 1;
                node->prefixMetadata.at(i).high += 1;
            }
        }
    }
    if (!equal) {
        int pfx_len = node->prefixMetadata[pfx_pos].prefix->size;
        InsertKey(node, insertpos, key + pfx_len, keylen - pfx_len);
    }
    return insertpos;
}

int BPTreeDB2::split_point(DB2Node *node) {
    int bestsplit = node->size / 2;
    return bestsplit;
}

void BPTreeDB2::do_split_node(DB2Node *node, DB2Node *right, int splitpos, bool isleaf) {
    // split node in splitpos, write to node-right
    // node: [0, split), right: [split, node->size)
    vector<PrefixMetaData> metadatas = node->prefixMetadata;
    vector<PrefixMetaData> leftmetadatas;
    vector<PrefixMetaData> rightmetadatas;
    PrefixMetaData leftmetadata, rightmetadata;
    int rightindex = 0;
    Data *splitprefix;

    // split the metadata
    for (auto pfx : metadatas) {
        // split occurs within this pfx
        if (splitpos >= pfx.low && splitpos <= pfx.high) {
            splitprefix = pfx.prefix;

            if (splitpos != pfx.low) {
                leftmetadata = PrefixMetaData(pfx.prefix->addr(), pfx.prefix->size, pfx.low, splitpos - 1);
                leftmetadatas.push_back(leftmetadata);
            }

            int rsize = isleaf ? pfx.high - splitpos : pfx.high - (splitpos + 1);
            if (rsize >= 0) {
                rightmetadata = PrefixMetaData(pfx.prefix->addr(), pfx.prefix->size, rightindex, rightindex + rsize);
                rightindex = rightindex + rsize + 1;
                rightmetadatas.push_back(rightmetadata);
            }
        }
        // to the right
        else if (splitpos <= pfx.low) {
            int size = pfx.high - pfx.low;
            rightmetadata = PrefixMetaData(pfx.prefix->addr(), pfx.prefix->size,
                                           rightindex, rightindex + size);
            rightindex += size + 1;
            rightmetadatas.push_back(rightmetadata);
        }
        // to the left
        else {
            leftmetadatas.push_back(pfx);
        }
    }

    // Split the keys to the new page

    char *l_base = NewPage();

    int l_usage = 0, r_usage = 0;
    uint16_t *l_idx = new uint16_t[kNumberBound];
    uint8_t *l_size = new uint8_t[kNumberBound];
    // l_size copy can be omitted, add for simplier codes
    CopyKeyToPage(node, 0, splitpos, l_base, l_usage, l_idx, l_size);
    if (isleaf) {
        CopyKeyToPage(node, splitpos, node->size,
                      right->base, right->memusage, right->keys_offset, right->keys_size);
        right->size = node->size - splitpos;
    }
    else {
        CopyKeyToPage(node, splitpos + 1, node->size,
                      right->base, right->memusage, right->keys_offset, right->keys_size);
        right->size = node->size - splitpos - 1;
    }
    right->prefixMetadata = rightmetadatas;
    right->IS_LEAF = isleaf;

    node->size = splitpos;
    node->memusage = l_usage;
    node->prefixMetadata = leftmetadatas;
    UpdateBase(node, l_base);
    UpdateOffset(node, l_idx);
    // no need to update size

    // Set prev and next pointers
    DB2Node *next = node->next;
    node->next = right;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;

    return;
}

splitReturnDB2 BPTreeDB2::split_nonleaf(DB2Node *node, int pos, splitReturnDB2 childsplit) {
    splitReturnDB2 newsplit;
    DB2Node *right = new DB2Node;
    // string promotekey = childsplit.promotekey;

    bool equal = false;
    int insertpos = insert_prefix_and_key(node,
                                          childsplit.promotekey->addr(),
                                          childsplit.promotekey->size, equal);
    InsertNode(node, insertpos + 1, childsplit.right);
    // vector<Key_c> allkeys;
    // vector<DB2Node *> allptrs;

    // for (int i = 0; i < insertpos; i++) {
    //     allkeys.push_back(node->keys.at(i));
    // }
    // // Non-leaf nodes won't have duplicates
    // allkeys.push_back(Key_c(string_to_char(promotekey), rid));
    // for (int i = insertpos; i < node->size; i++) {
    //     allkeys.push_back(node->keys.at(i));
    // }

    // for (int i = 0; i < insertpos + 1; i++) {
    //     allptrs.push_back(node->ptrs.at(i));
    // }
    // allptrs.push_back(childsplit.right);
    // for (int i = insertpos + 1; i < node->size + 1; i++) {
    //     allptrs.push_back(node->ptrs.at(i));
    // }

    int split = split_point(node);
    Data splitkey = Data(GetKey(node, split), node->keys_size[split]);
    // cout << "Best Split point " << split << " size " << allkeys.size() << endl;

    do_split_node(node, right, split, false);

    // vector<Key_c> leftkeys;
    // vector<Key_c> rightkeys;
    vector<DB2Node *> leftptrs;
    // vector<DB2Node *> rightptrs;
    // copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    // copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(node->ptrs.begin(), node->ptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(node->ptrs.begin() + split + 1, node->ptrs.end(), back_inserter(right->ptrs));

    // db2split splitresult = perform_split(node, split, false);
    // node->prefixMetadata = splitresult.leftmetadatas;
    // right->prefixMetadata = splitresult.rightmetadatas;
    // newsplit.promotekey = splitresult.splitprefix + allkeys.at(split).value;

    // node->size = leftkeys.size();
    // node->keys = leftkeys;
    right->ptr_cnt = node->ptr_cnt - (split + 1);
    node->ptr_cnt = split + 1;
    node->ptrs = leftptrs;

    // right->size = rightkeys.size();
    // right->IS_LEAF = false;
    // right->keys = rightkeys;
    // right->ptrs = rightptrs;

    // Set prev and next pointers
    // DB2Node *next = node->next;
    // right->prev = node;
    // right->next = next;
    // if (next)
    //     next->prev = right;
    // node->next = right;
    PrefixMetaData rf_pfx = right->prefixMetadata[0];
    int split_len = rf_pfx.prefix->size + splitkey.size;
    char *split_decomp = new char[split_len + 1];
    strncpy(split_decomp, rf_pfx.prefix->addr(), rf_pfx.prefix->size);
    strcpy(split_decomp + rf_pfx.prefix->size, splitkey.addr());

    newsplit.promotekey = new Data(split_decomp, split_len);
    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnDB2 BPTreeDB2::split_leaf(DB2Node *node, char *newkey, int keylen) {
    splitReturnDB2 newsplit;
    DB2Node *right = new DB2Node;
    int insertpos;
    bool equal = false;
#ifndef DUPKEY
    insertpos = insert_prefix_and_key(node, newkey, keylen, equal);

#else
    int rid = rand();
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
#endif
    int split = split_point(node);

    // vector<Key_c> leftkeys;
    // vector<Key_c> rightkeys;
    // copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    // copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));

    do_split_node(node, right, split, true);
    // db2split splitresult = perform_split(node, right, split, true);
    // node->prefixMetadata = splitresult.leftmetadatas;
    // right->prefixMetadata = splitresult.rightmetadatas;

    // node->size = leftkeys.size();
    // node->keys = leftkeys;

    // right->size = rightkeys.size();
    // right->IS_LEAF = true;
    // right->keys = rightkeys;

    // // Set prev and next pointers
    // DB2Node *next = node->next;
    // right->prev = node;
    // right->next = next;
    // if (next)
    //     next->prev = right;
    // node->next = right;

    // string firstright = right->keys.at(0).value;
    // PrefixMetaData firstrightmetadata = right->prefixMetadata.at(0);
    // firstright = firstrightmetadata.prefix + firstright;
    PrefixMetaData rf_pfx = right->prefixMetadata[0];
    int rf_len = rf_pfx.prefix->size + right->keys_size[0];
    char *rightfirst = new char[rf_len + 1];
    strncpy(rightfirst, rf_pfx.prefix->addr(), rf_pfx.prefix->size);
    strcpy(rightfirst + rf_pfx.prefix->size, GetKey(right, 0));

    newsplit.promotekey = new Data(rightfirst, rf_len);
    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeDB2::check_split_condition(DB2Node *node, int keylen) {
    int currspace = 0;
#ifdef SPLIT_STRATEGY_SPACE
    // for (uint32_t i = 0; i < node->prefixMetadata.size(); i++) {
    //     currspace += node->prefixMetadata.at(i).prefix.length();
    // }
    // for (uint32_t i = 0; i < node->keys.size(); i++) {
    //     currspace += node->keys.at(i).getSize();
    // }
    // return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;

    // Just simplify, may need to consider the prefix size
    if (node->memusage + 2 * keylen >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    return false;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeDB2::search_insert_pos(DB2Node *cursor, const char *key, int keylen,
                                 int low, int high, bool &equal) {
    // Search pos in node when insert, or search non-leaf node when search
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = char_cmp(key, GetKey(cursor, mid), keylen);
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

DB2Node *BPTreeDB2::search_leaf_node(DB2Node *searchroot, const char *key, int keylen) {
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
        int pos = -1;
        int metadatapos = find_prefix_pos(cursor, key, keylen, true);
        if (metadatapos == cursor->prefixMetadata.size()) {
            pos = cursor->size;
        }
        else {
            PrefixMetaData currentMetadata = cursor->prefixMetadata[metadatapos];
            Data *prefix = currentMetadata.prefix;
            // int prefixcmp = key.compare(0, prefix.length(), prefix);
            int prefixcmp = strncmp(key, prefix->addr(), prefix->size);
            if (prefixcmp != 0) {
                pos = prefixcmp > 0 ? currentMetadata.high + 1 : currentMetadata.low;
            }
            else {
                // searchkey = key.substr(prefix.length());
                pos = search_insert_pos(cursor, key + prefix->size, keylen - prefix->size,
                                        currentMetadata.low, currentMetadata.high, equal);
            }
        }
        cursor = cursor->ptrs.at(pos);
    }
    return cursor;
}

DB2Node *BPTreeDB2::search_leaf_node_for_insert(DB2Node *searchroot, const char *key, int keylen, DB2Node **path, int &path_level) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    DB2Node *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        // string_view searchkey = key;
        path[path_level++] = cursor;
        int pos = -1;
        int metadatapos = find_prefix_pos(cursor, key, keylen, true);
        if (metadatapos == cursor->prefixMetadata.size()) {
            pos = cursor->size;
        }
        else {
            PrefixMetaData currentMetadata = cursor->prefixMetadata[metadatapos];
            Data *prefix = currentMetadata.prefix;
            // int prefixcmp = key.compare(0, prefix.length(), prefix);
            int prefixcmp = strncmp(key, prefix->addr(), prefix->size);
            if (prefixcmp != 0) {
                pos = prefixcmp > 0 ? currentMetadata.high + 1 : currentMetadata.low;
            }
            else {
                // searchkey = key.substr(prefix.length());
                pos = search_insert_pos(cursor, key + prefix->size, keylen - prefix->size,
                                        currentMetadata.low, currentMetadata.high, equal);
            }
        }
        if (pos >= cursor->ptrs.size() || pos < 0)
            cout << "wrong pos" << endl;
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

int BPTreeDB2::search_in_leaf(DB2Node *cursor, const char *key, int keylen, int low, int high) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
#ifdef DUPKEY
        int cmp = char_cmp_old(key, cursor->keys.at(mid).value);
#else
        int cmp = char_cmp(key, GetKey(cursor, mid), keylen);
#endif
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

void BPTreeDB2::getSize(DB2Node *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                        int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
        int prefixSize = 0;
        for (uint32_t i = 0; i < cursor->prefixMetadata.size(); i++) {
            currSize += cursor->prefixMetadata.at(i).prefix->size;
            prefixSize += cursor->prefixMetadata.at(i).prefix->size;
        }
        for (int i = 0; i < cursor->size; i++) {
            currSize += cursor->keys_size[i];
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