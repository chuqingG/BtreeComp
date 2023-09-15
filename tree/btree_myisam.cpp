#include "btree_myisam.h"
#include "./compression/compression_myisam.cpp"

// Initialise the BPTreeMyISAM NodeMyISAM
BPTreeMyISAM::BPTreeMyISAM(bool non_leaf_compression) {
    _root = NULL;
    max_level = 1;
    non_leaf_comp = non_leaf_compression;
}

void deletefrom(NodeMyISAM *node) {
    if (node->IS_LEAF)
        delete node;
    else {
        for (auto child : node->ptrs) {
            deletefrom(child);
        }
        delete node;
    }
    return;
}

// Destructor of BPTreeMyISAM tree
BPTreeMyISAM::~BPTreeMyISAM() {
    deletefrom(_root);
    // delete _root;
}

// Function to get the root NodeMyISAM
NodeMyISAM *BPTreeMyISAM::getRoot() {
    return _root;
}

// Function to find any element
// in B+ Tree
int BPTreeMyISAM::search(const char *key) {
    int keylen = strlen(key);
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(_root, key, keylen);
    if (leaf == nullptr)
        return -1;
    return prefix_search(leaf, key, keylen);
}

void BPTreeMyISAM::insert(char *key) {
    int keylen = strlen(key);
    if (_root == nullptr) {
        _root = new NodeMyISAM;
        // int rid = rand();
        // _root->keys.push_back(KeyMyISAM(x, 0, rid));
        // _root->size = 1;
        InsertKeyMyISAM(_root, 0, key, keylen, 0);
        _root->IS_LEAF = true;
        return;
    }
    int path_level = 0;
    NodeMyISAM *search_path[max_level];
    NodeMyISAM *leaf = search_leaf_node_for_insert(_root, key, keylen, search_path, path_level);
    insert_leaf(leaf, search_path, path_level, key, keylen);
}
/* Adapted from MyISAM code base
   Based on the implementation of _mi_prefix_search in
   {https://github.com/mysql/mysql-server/blob/a246bad76b9271cb4333634e954040a970222e0a/storage/myisam/mi_search.cc#L277}
*/
int BPTreeMyISAM::prefix_search(NodeMyISAM *cursor, const char *key, int keylen) {
    /*  matched: how many chars from prefix were already matched
     *  len: length of previous key unpacked
     */
    int len = 0, matched = 0, idx = 0;

    while (idx < cursor->size) {
        MyISAMhead *head = GetHeaderMyISAM(cursor, idx);
        len = head->pfx_len + head->key_len;

        if (matched >= head->pfx_len) {
            /* We have to compare. But we can still skip part of the key */
            // uint left;

            /*
            If prefix_len > cmplen then we are in the end-space comparison
            phase. Do not try to access the key any more ==> left= 0.
            */
            uint8_t left = ((len <= keylen) ? head->key_len :
                                              ((head->pfx_len < keylen) ? keylen - head->pfx_len : 0));

            // compare the compressed k[idx] with key[idx.pfx_len: ]
            // if they have a common pfx of length n, then subtract n from left
            int common = 0;
            int cmp_result = 0;
            char *pagekey = PageOffset(cursor, head->key_offset);
            while (common < left) {
                if ((cmp_result = pagekey[common] - key[common + unsigned(head->pfx_len)]))
                    break;
                common++;
            }

            if (cmp_result > 0) /* mismatch */
                break;
            if (cmp_result == 0) {
                cmp_result = len - keylen;
                if (cmp_result > 0)
                    break;
                if (cmp_result == 0) {
                    return idx;
                }
            }
            matched = head->pfx_len + common;
        }
        idx++;
    }
    return -1;
}

int BPTreeMyISAM::prefix_insert(NodeMyISAM *cursor, const char *key, int keylen, bool &equal) {
    /* matched: how many chars from prefix were already matched
     *  len: length of previous key unpacked
     */
    int len = 0, matched = 0, idx = 0;

    while (idx < cursor->size) {
        MyISAMhead *head = GetHeaderMyISAM(cursor, idx);
        len = head->pfx_len + head->key_len;

        if (matched >= head->pfx_len) {
            /* We have to compare. But we can still skip part of the key */
            // uint8_t left;
            /*
            If prefix_len > cmplen then we are in the end-space comparison
            phase. Do not try to access the key any more ==> left= 0.
            */
            uint8_t left = ((len <= keylen) ? head->key_len :
                                              ((head->pfx_len < keylen) ? keylen - head->pfx_len : 0));

            // compare the compressed k[idx] with key[idx.pfx_len: ]
            // if they have a common pfx of length n, then subtract n from left
            int common = 0;
            int cmp_result = 0;
            char *pagekey = PageOffset(cursor, head->key_offset);
            while (common < left) {
                if ((cmp_result = pagekey[common] - key[common + unsigned(head->pfx_len)]))
                    break;
                common++;
            }

            if (cmp_result > 0) /* mismatch */
                return idx;
            if (cmp_result == 0) {
                cmp_result = len - keylen;
                if (cmp_result > 0)
                    return idx;
                if (cmp_result == 0) {
                    equal = true;
                    return idx + 1;
                }
            }
            // matched -= left;
            matched = head->pfx_len + common;
        }
        idx++;
    }
    return cursor->size;
}

void BPTreeMyISAM::insert_nonleaf(NodeMyISAM *node, NodeMyISAM **path, int parentlevel,
                                  splitReturnMyISAM *childsplit) {
    if (check_split_condition(node, childsplit->promotekey.size)) {
        NodeMyISAM *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturnMyISAM currsplit = split_nonleaf(node, parentlevel, childsplit);
        if (node == _root) {
            NodeMyISAM *newRoot = new NodeMyISAM;

            InsertNode(newRoot, 0, currsplit.left);
            InsertKeyMyISAM(newRoot, 0,
                            currsplit.promotekey.addr, currsplit.promotekey.size, 0);
            InsertNode(newRoot, 1, currsplit.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            if (parent == nullptr)
                return;
            insert_nonleaf(parent, path, parentlevel - 1, &currsplit);
        }
    }
    else {
        Item *promotekey = &(childsplit->promotekey);
        int insertpos;
        bool equal = false;
        // int rid = rand();
        if (this->non_leaf_comp) {
            insertpos = prefix_insert(node, promotekey->addr, promotekey->size, equal);
        }
        else {
            insertpos = search_insert_pos(node, promotekey->addr, promotekey->size, 0, node->size - 1, equal);
        }

        // Insert the new key
        Item prevkey;
        if (!this->non_leaf_comp || insertpos == 0) {
            InsertKeyMyISAM(node, insertpos, promotekey->addr, promotekey->size, 0);
            // use key as placeholder of key_before_pos
            prevkey.addr = promotekey->addr;
        }
        else {
            get_full_key(node, insertpos - 1, prevkey);
            uint8_t pfx_len = get_common_prefix_len(prevkey.addr, promotekey->addr,
                                                    prevkey.size, promotekey->size);
            InsertKeyMyISAM(node, insertpos, promotekey->addr + pfx_len, promotekey->size - pfx_len, pfx_len);
        }
        if (this->non_leaf_comp && !equal)
            update_next_prefix(node, insertpos, prevkey.addr, promotekey->addr, promotekey->size);

        InsertNode(node, insertpos + 1, childsplit->right);
    }
}

void BPTreeMyISAM::insert_leaf(NodeMyISAM *leaf, NodeMyISAM **path, int path_level,
                               char *key, int keylen) {
    if (check_split_condition(leaf, keylen)) {
        splitReturnMyISAM split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            NodeMyISAM *newRoot = new NodeMyISAM;

            InsertNode(newRoot, 0, split.left);
            InsertKeyMyISAM(newRoot, 0,
                            split.promotekey.addr, split.promotekey.size, 0);
            InsertNode(newRoot, 1, split.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            NodeMyISAM *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, &split);
        }
    }
    else {
        bool equal = false;
        int insertpos = prefix_insert(leaf, key, keylen, equal);

        // Insert the new key
        Item prevkey;
        if (insertpos == 0) {
            InsertKeyMyISAM(leaf, insertpos, key, keylen, 0);
            // use key as placeholder of key_before_pos
            prevkey.addr = key;
        }
        else {
            get_full_key(leaf, insertpos - 1, prevkey);
            uint8_t pfx_len = get_common_prefix_len(prevkey.addr, key,
                                                    prevkey.size, keylen);
            InsertKeyMyISAM(leaf, insertpos, key + pfx_len, keylen - pfx_len, pfx_len);
        }
        if (!equal)
            update_next_prefix(leaf, insertpos, prevkey.addr, key, keylen);
    }
}

int BPTreeMyISAM::split_point(NodeMyISAM *node) {
    // int size = allkeys.size();
    int bestsplit = node->size / 2;
    return bestsplit;
}

splitReturnMyISAM BPTreeMyISAM::split_nonleaf(NodeMyISAM *node, int pos, splitReturnMyISAM *childsplit) {
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;
    Item *promotekey = &(childsplit->promotekey);
    int insertpos;

    bool equal = false;
    if (this->non_leaf_comp) {
        insertpos = prefix_insert(node, promotekey->addr,
                                  promotekey->size, equal);
    }
    else {
        insertpos = search_insert_pos(node, promotekey->addr,
                                      promotekey->size, 0, node->size - 1, equal);
    }

    // insert the new key
    Item prevkey;
    if (!this->non_leaf_comp || insertpos == 0) {
        InsertKeyMyISAM(node, insertpos, promotekey->addr, promotekey->size, 0);
        // use key as placeholder of key_before_pos
        prevkey.addr = promotekey->addr;
    }
    else {
        get_full_key(node, insertpos - 1, prevkey);
        uint8_t pfx_len = get_common_prefix_len(prevkey.addr, promotekey->addr,
                                                prevkey.size, promotekey->size);
        InsertKeyMyISAM(node, insertpos, promotekey->addr + pfx_len,
                        promotekey->size - pfx_len, pfx_len);
    }
    if (this->non_leaf_comp) // nonleaf won't have duplicates
        update_next_prefix(node, insertpos, prevkey.addr,
                           promotekey->addr, promotekey->size);

    InsertNode(node, insertpos + 1, childsplit->right);

    int split = split_point(node);

    // 1. get separator
    if (this->non_leaf_comp)
        get_full_key(node, split, newsplit.promotekey);
    else {
        MyISAMhead *header_split = GetHeaderMyISAM(node, split);
        newsplit.promotekey.addr = PageOffset(node, header_split->key_offset);
        newsplit.promotekey.size = header_split->key_len;
    }

    if (!newsplit.promotekey.newallocated) {
        // make a copy, in case the memory in page is reallocated
        char *keycopy = new char[newsplit.promotekey.size + 1];
        strcpy(keycopy, newsplit.promotekey.addr);
        newsplit.promotekey.addr = keycopy;
        newsplit.promotekey.newallocated = true;
    }

    // 2. Substitute the first key in right part with its full key
    if (this->non_leaf_comp) {
        MyISAMhead *header_rf = GetHeaderMyISAM(node, split + 1);
        Item firstright_fullkey;
        get_full_key(node, split + 1, firstright_fullkey);
        strncpy(BufTop(node), firstright_fullkey.addr, firstright_fullkey.size);
        header_rf->key_len = firstright_fullkey.size;
        header_rf->pfx_len = 0;
        header_rf->key_offset = node->space_top;
        node->space_top += header_rf->key_len + 1;
    }

    // 3. copy the two parts into new pages
    char *leftbase = NewPage();
    SetEmptyPage(leftbase);
    char *rightbase = right->base;
    int left_top = 0, right_top = 0;

    CopyToNewPageMyISAM(node, 0, split, leftbase, left_top);
    CopyToNewPageMyISAM(node, split + 1, node->size, rightbase, right_top);

    // 4. update pointers
    vector<NodeMyISAM *> leftptrs;
    vector<NodeMyISAM *> rightptrs;
    copy(node->ptrs.begin(), node->ptrs.begin() + split + 1,
         back_inserter(leftptrs));
    copy(node->ptrs.begin() + split + 1, node->ptrs.end(),
         back_inserter(rightptrs));

    right->size = node->size - (split + 1);
    right->space_top = right_top;
    right->IS_LEAF = false;
    right->ptrs = rightptrs;
    right->ptr_cnt = node->size - split;

    node->size = split;
    node->space_top = left_top;
    UpdateBase(node, leftbase);
    node->ptrs = leftptrs;
    node->ptr_cnt = split + 1;

    // Set next pointers
    NodeMyISAM *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnMyISAM BPTreeMyISAM::split_leaf(NodeMyISAM *node, char *newkey, int keylen) {
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;

    bool equal = false;
    int insertpos = prefix_insert(node, newkey, keylen, equal);

    // insert the new key
    Item prevkey;
    if (insertpos == 0) {
        InsertKeyMyISAM(node, insertpos, newkey, keylen, 0);
        // use key as placeholder of key_before_pos
        prevkey.addr = newkey;
    }
    else {
        get_full_key(node, insertpos - 1, prevkey);
        uint8_t pfx_len = get_common_prefix_len(prevkey.addr, newkey,
                                                prevkey.size, keylen);
        InsertKeyMyISAM(node, insertpos, newkey + pfx_len, keylen - pfx_len, pfx_len);
    }
    if (!equal)
        update_next_prefix(node, insertpos, prevkey.addr, newkey, keylen);

    int split = split_point(node);

    // 1. get separator
    get_full_key(node, split, newsplit.promotekey);
    if (!newsplit.promotekey.newallocated) {
        // make a copy, in case the memory in page is reallocated
        char *keycopy = new char[newsplit.promotekey.size + 1];
        strcpy(keycopy, newsplit.promotekey.addr);
        newsplit.promotekey.addr = keycopy;
        newsplit.promotekey.newallocated = true;
    }

    // 2. Substitute the first key in right part with its full key
    MyISAMhead *header_rf = GetHeaderMyISAM(node, split);
    strncpy(BufTop(node), newsplit.promotekey.addr, newsplit.promotekey.size);
    header_rf->key_len = newsplit.promotekey.size;
    header_rf->pfx_len = 0;
    header_rf->key_offset = node->space_top;
    node->space_top += header_rf->key_len + 1;

    // 3. copy the two parts into new pages
    char *leftbase = NewPage();
    SetEmptyPage(leftbase);
    char *rightbase = right->base;
    int left_top = 0, right_top = 0;

    CopyToNewPageMyISAM(node, 0, split, leftbase, left_top);
    CopyToNewPageMyISAM(node, split, node->size, rightbase, right_top);

    right->size = node->size - split;
    right->space_top = right_top;
    right->IS_LEAF = true;

    node->size = split;
    node->space_top = left_top;
    UpdateBase(node, leftbase);

    // Set prev and next pointers
    NodeMyISAM *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeMyISAM::check_split_condition(NodeMyISAM *node, int keylen) {
    int currspace = node->space_top + node->size * sizeof(MyISAMhead);
    // Update prefix need more space, the newkey, the following key, the decompressed split point
    int splitcost = keylen + sizeof(MyISAMhead) + 2 * max(keylen, APPROX_KEY_SIZE);

    if (currspace + splitcost >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    else
        return false;
}

int BPTreeMyISAM::search_insert_pos(NodeMyISAM *cursor, const char *key, int keylen,
                                    int low, int high, bool &equal) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Item curkey;
        if (!cursor->IS_LEAF && !this->non_leaf_comp) {
            // non-compressed
            MyISAMhead *header = GetHeaderMyISAM(cursor, mid);
            curkey.addr = PageOffset(cursor, header->key_offset);
            curkey.size = header->key_len;
        }
        else {
            get_full_key(cursor, mid, curkey);
        }

        int cmp = char_cmp_new(key, curkey.addr, keylen, curkey.size);
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

NodeMyISAM *BPTreeMyISAM::search_leaf_node(NodeMyISAM *searchroot, const char *key, int keylen) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeMyISAM *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        int pos = prefix_insert(cursor, key, keylen, equal);
        cursor = cursor->ptrs.at(pos);
    }
    return cursor;
}

NodeMyISAM *BPTreeMyISAM::search_leaf_node_for_insert(NodeMyISAM *searchroot, const char *key, int keylen,
                                                      NodeMyISAM **path, int &path_level) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeMyISAM *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        path[path_level++] = cursor;
        int pos = prefix_insert(cursor, key, keylen, equal);
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

/*
================================================
=============statistic function & printer=======
================================================
*/

void BPTreeMyISAM::getSize(NodeMyISAM *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                           int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
        int prefixSize = 0;
        for (int i = 0; i < cursor->size; i++) {
            MyISAMhead *header = GetHeaderMyISAM(cursor, i);
            currSize += header->key_len + sizeof(MyISAMhead);
            prefixSize += sizeof(header->pfx_len);
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

int BPTreeMyISAM::getHeight(NodeMyISAM *cursor) {
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

void BPTreeMyISAM::printTree(NodeMyISAM *x, vector<bool> flag, bool compressed, int depth, bool isLast) {
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
        printKeys_myisam(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast) {
        cout << "+--- ";
        printKeys_myisam(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }
    else {
        cout << "+--- ";
        printKeys_myisam(x, compressed);
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