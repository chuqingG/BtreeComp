#include "btree_myisam.h"
#include "./compression/compression_myisam.cpp"

// Initialise the BPTreeMyISAM NodeMyISAM
BPTreeMyISAM::BPTreeMyISAM(bool non_leaf_compression) {
    _root = NULL;
    max_level = 1;
    non_leaf_comp = non_leaf_compression;
}

// Destructor of BPTreeMyISAM tree
BPTreeMyISAM::~BPTreeMyISAM() {
    delete _root;
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
    NodeMyISAM *leaf = search_leaf_node(_root, key, parents);
    if (leaf == nullptr)
        return -1;
    return prefix_search(leaf, key);
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
int BPTreeMyISAM::prefix_search(NodeMyISAM *cursor, string_view key) {
    int cmplen;
    int len, matched;
    int my_flag;

    int ind = 0;
    cmplen = key.length();

    matched = 0; /* how many chars from prefix were already matched */
    len = 0;     /* length of previous key unpacked */

    while (ind < cursor->size) {
        int prefix = cursor->keys.at(ind).getPrefix();
        string suffix = cursor->keys.at(ind).value;
        len = prefix + suffix.length();

        string pagekey = suffix;

        if (matched >= prefix) {
            /* We have to compare. But we can still skip part of the key */
            uint left;

            /*
            If prefix_len > cmplen then we are in the end-space comparison
            phase. Do not try to access the key any more ==> left= 0.
            */
            left =
                ((len <= cmplen) ? suffix.length() : ((prefix < cmplen) ? cmplen - prefix : 0));

            matched = prefix + left;

            int ppos = 0;
            int kpos = prefix;
            for (my_flag = 0; left; left--) {
                string c1(1, pagekey.at(ppos));
                string c2(1, key.at(kpos));
                if ((my_flag = c1.compare(c2)))
                    break;
                kpos++;
                ppos++;
            }
            if (my_flag > 0) /* mismatch */
                break;
            if (my_flag == 0) {
                if (len < cmplen) {
                    my_flag = -1;
                }
                else if (len > cmplen) {
                    my_flag = 1;
                }

                if (my_flag > 0)
                    break;
                if (my_flag == 0)
                    return ind;
            }
            matched -= left;
        }
        ind++;
    }
    return -1;
}

int BPTreeMyISAM::prefix_insert(NodeMyISAM *cursor, const char *key, int keylen, bool &equal) {
    // int cmplen;
    /* matched: how many chars from prefix were already matched
     *  len: length of previous key unpacked
     */
    int len = 0, matched = 0, idx = 0;
    // int my_flag;

    // int ind = 0;
    // cmplen = key.length();
    // matched = 0; /* how many chars from prefix were already matched */
    // len = 0;     /* length of previous key unpacked */

    while (idx < cursor->size) {
        MyISAMhead *head = GetHeaderMyISAM(cursor, idx);
        // int prefix = cursor->keys.at(ind).getPrefix();
        // string suffix = cursor->keys.at(ind).value;
        len = head->pfx_len + head->key_len;

        // string pagekey = suffix;

        if (matched >= head->pfx_len) {
            /* We have to compare. But we can still skip part of the key */
            // uint8_t left;
            /*
            If prefix_len > cmplen then we are in the end-space comparison
            phase. Do not try to access the key any more ==> left= 0.
            */
            uint8_t left = ((len <= keylen) ? head->key_len :
                                              ((head->pfx_len < keylen) ? keylen - head->pfx_len : 0));

            // matched = head->pfx_len + left;

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
            // left -= common;
            // int ppos = 0;
            // int kpos = prefix;
            // for (my_flag = 0; left; left--) {
            //     string c1(1, pagekey.at(ppos));
            //     string c2(1, key.at(kpos));
            //     if ((my_flag = c1.compare(c2)))
            //         break;
            //     ppos++;
            //     kpos++;
            // }

            if (cmp_result > 0) /* mismatch */
                return idx;
            if (cmp_result == 0) {
                // if (len < keylen) {
                //     my_flag = -1;
                // }
                // else if (len > keylen) {
                //     my_flag = 1;
                // }
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

void BPTreeMyISAM::insert_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, int pos, splitReturnMyISAM childsplit) {
    if (check_split_condition(node)) {
        NodeMyISAM *parent = nullptr;
        if (pos >= 0) {
            parent = parents.at(pos);
        }
        splitReturnMyISAM currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == _root) {
            NodeMyISAM *newRoot = new NodeMyISAM;
            int rid = rand();
            newRoot->keys.push_back(KeyMyISAM(currsplit.promotekey, 0, rid));
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
        int insertpos;
        bool equal = false;
        int rid = rand();
        if (this->non_leaf_comp) {
            insertpos = prefix_insert(node, promotekey, equal);
        }
        else {
            insertpos = insert_binary(node, promotekey, 0, node->size - 1, equal);
        }
        vector<KeyMyISAM> allkeys;
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }
        if (this->non_leaf_comp) {
            if (insertpos > 0) {
                string prev_key = get_key(node, insertpos - 1);
                int pfx = compute_prefix_myisam(prev_key, promotekey);
                string compressed = promotekey.substr(pfx);
                allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
            }
            else {
                allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
            }
        }
        else {
            allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
        }

        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }

        vector<NodeMyISAM *> allptrs;
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

        if (this->non_leaf_comp)
            build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
    }
}

void BPTreeMyISAM::insert_leaf(NodeMyISAM *leaf, NodeMyISAM **path, int path_level,
                               char *key, int keylen) {
    if (check_split_condition(leaf, keylen)) {
        splitReturnMyISAM split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            NodeMyISAM *newRoot = new NodeMyISAM;
            // int rid = rand();
            // newRoot->keys.push_back(KeyMyISAM(split.promotekey, 0, rid));
            // newRoot->ptrs.push_back(split.left);
            // newRoot->ptrs.push_back(split.right);
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
        // int insertpos;
        // int rid = rand();
        bool equal = false;
        int insertpos = prefix_insert(leaf, key, keylen, equal);

        // Insert the new key
        WTitem prevkey;
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

        // vector<KeyMyISAM> allkeys;
        // if (equal) {
        //     allkeys = leaf->keys;
        //     allkeys.at(insertpos - 1).addRecord(rid);
        // }
        // else {
        //     for (int i = 0; i < insertpos; i++) {
        //         allkeys.push_back(leaf->keys.at(i));
        //     }
        //     if (insertpos > 0) {
        //         string prev_key = get_key(leaf, insertpos - 1);
        //         int pfx = compute_prefix_myisam(prev_key, key);
        //         string compressed = key.substr(pfx);
        //         allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
        //     }
        //     else {
        //         allkeys.push_back(KeyMyISAM(key, 0, rid));
        //     }

        //     for (int i = insertpos; i < leaf->size; i++) {
        //         allkeys.push_back(leaf->keys.at(i));
        //     }
        // }
        // leaf->keys = allkeys;
        // if (!equal) {
        //     leaf->size = leaf->size + 1;
        //     build_page_prefixes(leaf, insertpos, key); // Populate new prefixes
        // }
    }
}

int BPTreeMyISAM::split_point(NodeMyISAM *node) {
    // int size = allkeys.size();
    int bestsplit = node->size / 2;
    return bestsplit;
}

splitReturnMyISAM BPTreeMyISAM::split_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> parents, int pos, splitReturnMyISAM childsplit) {
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;
    string promotekey = childsplit.promotekey;
    int insertpos;
    string splitprefix;
    int rid = rand();
    bool equal = false;
    if (this->non_leaf_comp) {
        insertpos = prefix_insert(node, promotekey, equal);
    }
    else {
        insertpos = insert_binary(node, promotekey, 0, node->size - 1, equal);
    }

    vector<KeyMyISAM> allkeys;
    vector<NodeMyISAM *> allptrs;

    for (int i = 0; i < insertpos; i++) {
        allkeys.push_back(node->keys.at(i));
    }
    if (this->non_leaf_comp) {
        if (insertpos > 0) {
            string prev_key = get_key(node, insertpos - 1);
            int pfx = compute_prefix_myisam(prev_key, promotekey);
            string compressed = promotekey.substr(pfx);
            allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
        }
        else {
            allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
        }
    }
    else {
        allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
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

    int split = split_point(allkeys);

    // cout << "Best Split point " << split << " size " << allkeys.size() << endl;

    vector<KeyMyISAM> leftkeys;
    vector<KeyMyISAM> rightkeys;
    vector<NodeMyISAM *> leftptrs;
    vector<NodeMyISAM *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    string splitkey;
    string firstright;

    if (this->non_leaf_comp) {
        firstright = get_uncompressed_key_before_insert(node, split + 1, insertpos, promotekey, equal);
        rightkeys.at(0) = KeyMyISAM(firstright, 0, rightkeys.at(0).ridList);
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
    NodeMyISAM *next = node->next;
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

splitReturnMyISAM BPTreeMyISAM::split_leaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, string newkey) {
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;
    int rid = rand();
    bool equal = false;
    int insertpos = prefix_insert(node, newkey, equal);

    vector<KeyMyISAM> allkeys;
    if (equal) {
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }

        if (insertpos > 0) {
            string prev_key = get_key(node, insertpos - 1);
            int pfx = compute_prefix_myisam(prev_key, newkey);
            string compressed = newkey.substr(pfx);
            allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
        }
        else {
            allkeys.push_back(KeyMyISAM(newkey, 0, rid));
        }

        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }
    }

    int split = split_point(allkeys);

    vector<KeyMyISAM> leftkeys;
    vector<KeyMyISAM> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));

    string firstright;
    firstright = get_uncompressed_key_before_insert(node, split, insertpos, newkey, equal);
    rightkeys.at(0) = KeyMyISAM(firstright, 0, rightkeys.at(0).ridList);
    newsplit.promotekey = firstright;

    node->size = leftkeys.size();
    node->keys = leftkeys;

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;

    // Set prev and next pointers
    NodeMyISAM *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    if (!equal && insertpos < split) {
        build_page_prefixes(node, insertpos, newkey); // Populate new prefixes
    }
    build_page_prefixes(right, 0, firstright); // Populate new prefixes

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

int BPTreeMyISAM::insert_binary(NodeMyISAM *cursor, string_view key, int low, int high, bool &equal) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        string pagekey = cursor->keys.at(mid).value;
        int cmp = lex_compare(string(key), cursor->keys.at(mid).value);
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
        string_view searchkey = key;
        parents.push_back(cursor);
        int pos;
        pos = prefix_insert(cursor, searchkey, equal);
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
        // string_view searchkey = key;
        // parents.push_back(cursor);
        path[path_level++] = cursor;
        int pos = prefix_insert(cursor, key, keylen, equal);
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

int BPTreeMyISAM::search_binary(NodeMyISAM *cursor, string_view key, int low, int high) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        string pagekey = cursor->keys.at(mid).value;
        int cmp = lex_compare(string(key), pagekey);
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

void BPTreeMyISAM::getSize(NodeMyISAM *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                           int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
        int prefixSize = 0;
        for (int i = 0; i < cursor->size; i++) {
            currSize += cursor->keys.at(i).getSize();
            prefixSize += cursor->keys.at(i).getPrefix() <= 127 ? 1 : 2;
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