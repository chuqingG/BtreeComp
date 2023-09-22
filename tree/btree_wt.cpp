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

void deletefrom(NodeWT *node) {
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

// Destructor of BPTreeWT tree
BPTreeWT::~BPTreeWT() {
    deletefrom(_root);
    // delete _root;
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
    return search_in_leaf(leaf, key, keylen, 0, leaf->size - 1, skiplow);
}

int BPTreeWT::searchRange(const char *kmin, const char *kmax) {
    int min_len = strlen(kmin);
    int max_len = strlen(kmax);
    // vector<NodeWT *> parents;
    uint8_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(_root, kmin, min_len, skiplow);
    if (leaf == nullptr)
        return 0;
    int pos = search_in_leaf(leaf, kmin, min_len, 0, leaf->size - 1, skiplow);
    if (pos == -1)
        return 0;
    int entries = 0;
    // string prevkey = min;
    Item prevkey, curkey;
    WThead *head_pos = GetHeaderWT(leaf, pos);
    // prevkey.addr = PageOffset(leaf, head_pos->key_offset);
    prevkey.addr = new char[min_len + 1];
    strcpy(prevkey.addr, kmin);
    prevkey.size = min_len;
    prevkey.newallocated = true;
    // Keep searching till value > max or we reach end of tree
    while (leaf != nullptr) {
        if (pos == leaf->size) {
            pos = 0;
            leaf = leaf->next;
            if (leaf == nullptr)
                break;
            WThead *head_first = GetHeaderWT(leaf, 0);
            prevkey.addr = PageOffset(leaf, head_first->key_offset);
            prevkey.size = head_first->key_len;
            continue;
        }

        // int pfxcmp = char_cmp_new(prevkey.addr, kmax, )
        WThead *h_pos = GetHeaderWT(leaf, pos);
        curkey.size = h_pos->pfx_len + h_pos->key_len;
        curkey.addr = new char[curkey.size + 1];
        curkey.newallocated = true;

        // build current key
        strncpy(curkey.addr, prevkey.addr, h_pos->pfx_len);
        strcpy(curkey.addr + h_pos->pfx_len, PageOffset(leaf, h_pos->key_offset));
        if (char_cmp_new(curkey.addr, kmax, curkey.size, max_len) > 0)
            break;
        // string leafkey = prevkey.substr(0, leaf->keys.at(pos).prefix) + leaf->keys.at(pos).value;
        // if (lex_compare(leafkey, max) > 0) {
        //     break;
        // }
        // prevkey = leafkey;
        // entries += leaf->keys.at(pos).ridList.size();
        // delete the previous allocated memory
        if (pos)
            delete prevkey.addr;
        prevkey = curkey;
        // prevkey.newallocated = true;
        entries++;
        pos++;
    }
    return entries;
}

void BPTreeWT::insert(char *x) {
    int keylen = strlen(x);
    if (_root == nullptr) {
        _root = new NodeWT();

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

void BPTreeWT::insert_nonleaf(NodeWT *node, NodeWT **path, int parentlevel, splitReturnWT *childsplit) {
    if (check_split_condition(node, childsplit->promotekey.size)) {
        NodeWT *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturnWT currsplit = split_nonleaf(node, parentlevel, childsplit);
        if (node == _root) {
            NodeWT *newRoot = new NodeWT();

            InsertNode(newRoot, 0, currsplit.left);
            InsertKeyWT(newRoot, 0,
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
        Item *splitkey = &(childsplit->promotekey);
        uint8_t skiplow = 0;
        // int rid = rand();
        bool equal = false;
        int insertpos = search_insert_pos(node, splitkey->addr, splitkey->size,
                                          0, node->size - 1, skiplow, equal);

        Item prevkey;
        if (this->non_leaf_comp && insertpos != 0) {
            // fetch the previous key, compute prefix_len
            get_full_key(node, insertpos - 1, prevkey);
            uint8_t pfx_len = get_common_prefix_len(prevkey.addr, splitkey->addr,
                                                    prevkey.size, splitkey->size);
            InsertKeyWT(node, insertpos, splitkey->addr + pfx_len,
                        splitkey->size - pfx_len, pfx_len);
        }
        else {
            InsertKeyWT(node, insertpos, splitkey->addr, splitkey->size, 0);
            // use key as placeholder of key_before_pos
            prevkey.addr = splitkey->addr;
        }
        if (this->non_leaf_comp && !equal) {
            update_next_prefix(node, insertpos, prevkey.addr, splitkey->addr, splitkey->size);
#ifdef WT_OPTIM
            record_page_prefix_group(node);
#endif
        }

        InsertNode(node, insertpos + 1, childsplit->right);

        // if (this->non_leaf_comp) {
        //     // Rebuild prefixes if a new item is inserted in position 0
        //     build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
        // }
    }
}

void BPTreeWT::insert_leaf(NodeWT *leaf, NodeWT **path, int path_level, char *key, int keylen) {
    if (check_split_condition(leaf, keylen)) {
        splitReturnWT split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            NodeWT *newRoot = new NodeWT();

            InsertNode(newRoot, 0, split.left);
            InsertKeyWT(newRoot, 0,
                        split.promotekey.addr, split.promotekey.size, 0);
            InsertNode(newRoot, 1, split.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            NodeWT *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, &split);
        }
    }
    else {
        uint8_t skiplow = 0;
        bool equal = false;
        int insertpos = search_insert_pos(leaf, key, keylen,
                                          0, leaf->size - 1, skiplow, equal);

        Item prevkey;
        if (insertpos == 0) {
            InsertKeyWT(leaf, insertpos, key, keylen, 0);
            // use key as placeholder of key_before_pos
            prevkey.addr = key;
        }
        else {
            // fetch the previous key, compute prefix_len
            get_full_key(leaf, insertpos - 1, prevkey);
            uint8_t pfx_len = get_common_prefix_len(prevkey.addr, key,
                                                    prevkey.size, keylen);
            InsertKeyWT(leaf, insertpos, key + pfx_len, keylen - pfx_len, pfx_len);
        }
        if (!equal) {
            update_next_prefix(leaf, insertpos, prevkey.addr, key, keylen);
#ifdef WT_OPTIM
            record_page_prefix_group(leaf);
#endif
        }
    }
}

int BPTreeWT::split_point(NodeWT *node) {
    // int size = allkeys.size();
    int bestsplit = node->size / 2;
    return bestsplit;
}

splitReturnWT BPTreeWT::split_nonleaf(NodeWT *node, int pos, splitReturnWT *childsplit) {
    splitReturnWT newsplit;
    NodeWT *right = new NodeWT();
    Item *promotekey = &(childsplit->promotekey);

    uint8_t skiplow = 0;
    bool equal = false;
    int insertpos = search_insert_pos(node, promotekey->addr, promotekey->size,
                                      0, node->size - 1, skiplow, equal);

    Item prevkey;
    // Non-leaf nodes won't have duplicates
    if (this->non_leaf_comp && insertpos != 0) {
        // fetch the previous key, compute prefix_len
        get_full_key(node, insertpos - 1, prevkey);
        uint8_t pfx_len = get_common_prefix_len(prevkey.addr, promotekey->addr,
                                                prevkey.size, promotekey->size);
        InsertKeyWT(node, insertpos, promotekey->addr + pfx_len,
                    promotekey->size - pfx_len, pfx_len);
    }
    else {
        InsertKeyWT(node, insertpos, promotekey->addr, promotekey->size, 0);
        // use key as placeholder of key_before_pos
        prevkey.addr = promotekey->addr;
    }
    if (this->non_leaf_comp) {
        update_next_prefix(node, insertpos, prevkey.addr, promotekey->addr, promotekey->size);
#ifdef WT_OPTIM
        record_page_prefix_group(node);
#endif
    }
    InsertNode(node, insertpos + 1, childsplit->right);

    int split = split_point(node);

    // 1. get separator
    // Item firstright_fullkey = get_full_key(node, split);

    if (this->non_leaf_comp) {
        // separator = k[split]
        get_full_key(node, split, newsplit.promotekey);
    }
    else {
        WThead *header_split = GetHeaderWT(node, split);
        newsplit.promotekey.addr = PageOffset(node, header_split->key_offset);
        newsplit.promotekey.size = header_split->key_len;
    }
    // make a copy, the addr will be reallocated after write into new page
    char *keycopy = new char[newsplit.promotekey.size + 1];
    strncpy(keycopy, newsplit.promotekey.addr, newsplit.promotekey.size);
    keycopy[newsplit.promotekey.size] = '\0';
    newsplit.promotekey.addr = keycopy;
    newsplit.promotekey.newallocated = true;

    // 2. Substitute the first key in right part with its full key
    if (this->non_leaf_comp) {
        WThead *header_rf = GetHeaderWT(node, split + 1);
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

    CopyToNewPageWT(node, 0, split, leftbase, left_top);
    CopyToNewPageWT(node, split + 1, node->size, rightbase, right_top);

    // 4. update pointers
    vector<NodeWT *> leftptrs;
    vector<NodeWT *> rightptrs;

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
    NodeWT *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

#ifdef WT_OPTIM
    if (this->non_leaf_comp) {
        record_page_prefix_group(node);
        record_page_prefix_group(right);
    }
#endif
    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnWT BPTreeWT::split_leaf(NodeWT *node, char *newkey, int keylen) {
    splitReturnWT newsplit;
    NodeWT *right = new NodeWT();
    uint8_t skiplow = 0;

    bool equal = false;
    int insertpos = search_insert_pos(node, newkey, keylen,
                                      0, node->size - 1, skiplow, equal);
    // insert the newkey into page
    Item prevkey;
    if (insertpos == 0) {
        InsertKeyWT(node, insertpos, newkey, keylen, 0);
        // use key as placeholder of key_before_pos
        prevkey.addr = newkey;
    }
    else {
        // fetch the previous key, compute prefix_len
        get_full_key(node, insertpos - 1, prevkey);
        uint8_t pfx_len = get_common_prefix_len(prevkey.addr, newkey,
                                                prevkey.size, keylen);
        InsertKeyWT(node, insertpos, newkey + pfx_len, keylen - pfx_len, pfx_len);
    }
    if (!equal) {
        update_next_prefix(node, insertpos, prevkey.addr, newkey, keylen);
#ifdef WT_OPTIM
        record_page_prefix_group(node);
#endif
    }

    int split = split_point(node);

    // cout << "----------before split------------" << endl;
    // vector<bool> flag(node->size + 1);
    // printTree(node, flag, true);

    // 1. get separator
    Item firstright_fullkey;
    get_full_key(node, split, firstright_fullkey);
    if (this->suffix_comp) {
        Item lastleft_fullkey;
        get_full_key(node, split - 1, lastleft_fullkey);
        suffix_truncate(&lastleft_fullkey, &firstright_fullkey,
                        node->IS_LEAF, newsplit.promotekey);
    }
    else {
        newsplit.promotekey = firstright_fullkey;
    }
    // make a copy, the addr will be reallocated after write into new page
    char *keycopy = new char[newsplit.promotekey.size + 1];
    strncpy(keycopy, newsplit.promotekey.addr, newsplit.promotekey.size);
    keycopy[newsplit.promotekey.size] = '\0';
    newsplit.promotekey.addr = keycopy;
    newsplit.promotekey.newallocated = true;

    // 2. Substitute the first key in right part with its full key
    WThead *header_rf = GetHeaderWT(node, split);
    strncpy(BufTop(node), firstright_fullkey.addr, firstright_fullkey.size);
    header_rf->key_len = firstright_fullkey.size;
    header_rf->pfx_len = 0;
    header_rf->key_offset = node->space_top;
    node->space_top += header_rf->key_len + 1;

    // 3. copy the two parts into new pages
    char *leftbase = NewPage();
    SetEmptyPage(leftbase);
    char *rightbase = right->base;
    int left_top = 0, right_top = 0;

    CopyToNewPageWT(node, 0, split, leftbase, left_top);
    CopyToNewPageWT(node, split, node->size, rightbase, right_top);

    right->size = node->size - split;
    right->space_top = right_top;
    right->IS_LEAF = true;

    node->size = split;
    node->space_top = left_top;
    UpdateBase(node, leftbase);

    // Set next pointers
    NodeWT *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

#ifdef WT_OPTIM
    record_page_prefix_group(node);
    record_page_prefix_group(right);
#endif
    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeWT::check_split_condition(NodeWT *node, int keylen) {
    int currspace = node->space_top + node->size * sizeof(WThead);
    // Update prefix need more space, the newkey, the following key, the decompressed split point
    int splitcost = keylen + sizeof(WThead) + 2 * max(keylen, APPROX_KEY_SIZE);

    if (currspace + splitcost >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    else
        return false;
}

int BPTreeWT::search_insert_pos(NodeWT *cursor, const char *key, int keylen,
                                int low, int high, uint8_t &skiplow, bool &equal) {
    uint8_t skiphigh = 0;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Item curkey;
        if (!cursor->IS_LEAF && !this->non_leaf_comp) {
            // non-compressed
            WThead *header = GetHeaderWT(cursor, mid);
            curkey.addr = PageOffset(cursor, header->key_offset);
            curkey.size = header->key_len;
        }
        else {
            get_full_key(cursor, mid, curkey);
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

int BPTreeWT::search_in_leaf(NodeWT *cursor, const char *key, int keylen,
                             int low, int high, uint8_t &skiplow) {
    uint8_t skiphigh = 0;
    // uint8_t match;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Item curkey;
        get_full_key(cursor, mid, curkey);

        // string pagekey = extract_key(cursor, mid, this->non_leaf_comp);
        int cmp;
        uint8_t match = min(skiplow, skiphigh);
        cmp = char_cmp_skip(key, curkey.addr, keylen, curkey.size, &match);
        // cmp = lex_compare_skip(string(key), pagekey, &match);

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
            WThead *header = GetHeaderWT(cursor, i);
            currSize += header->key_len + sizeof(WThead);
            prefixSize += sizeof(header->pfx_len);
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