#include "btree_std.h"
#include "./compression/compression_std.cpp"

// Initialise the BPTree Node
BPTree::BPTree(bool head_compression, bool tail_compression) {
    _root = NULL;
    max_level = 1;
    head_comp = head_compression;
    tail_comp = tail_compression;
}

// Destructor of BPTreePkB tree
BPTree::~BPTree() {
    delete _root;
}

// Function to get the rootNode
Node *BPTree::getRoot() {
    return _root;
}

// Function to find any element
// in B+ Tree
int BPTree::search(const char *key) {
    int keylen = strlen(key);
    Node *leaf = search_leaf_node(_root, key, keylen);
    if (leaf == nullptr)
        return -1;
    if (this->head_comp) {
        return search_in_node(leaf, key + leaf->prefix->size, keylen - leaf->prefix->size,
                              0, leaf->size - 1, true);
    }
    else {
        return search_in_node(leaf, key, keylen, 0, leaf->size - 1, true);
    }
}

void BPTree::insert(char *x) {
    int keylen = strlen(x);
    if (_root == nullptr) {
        _root = new Node();
#ifdef DUPKEY
        int rid = rand();
        _root->keys.push_back(Key_c(x, rid));
        _root->size = 1;
#else
        InsertKeyStd(_root, 0, x, keylen);
#endif
        _root->IS_LEAF = true;
        return;
    }

    Node *search_path[max_level];
    int path_level = 0;
    Node *leaf = search_leaf_node_for_insert(_root, x, keylen, search_path, path_level);
    insert_leaf(leaf, search_path, path_level, x, keylen);
}

void BPTree::insert_nonleaf(Node *node, Node **path,
                            int parentlevel, splitReturn_new *childsplit) {
    if (check_split_condition(node, childsplit->promotekey.size)) {
        Node *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturn_new currsplit =
            split_nonleaf(node, parentlevel, childsplit);
        if (node == _root) {
            Node *newRoot = new Node();
#ifdef DUPKEY
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(currsplit.promotekey), rid));
#endif
            InsertNode(newRoot, 0, currsplit.left);
            InsertKeyStd(newRoot, 0, currsplit.promotekey.addr, currsplit.promotekey.size);
            InsertNode(newRoot, 1, currsplit.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            if (parent == nullptr) {
                return;
            }
            insert_nonleaf(parent, path, parentlevel - 1, &currsplit);
        }
    }
    else {
        WTitem *newkey = &(childsplit->promotekey);
        int insertpos;
        bool equal = false;
        if (this->head_comp) {
            insertpos = search_insert_pos(node, newkey->addr + node->prefix->size,
                                          newkey->size - node->prefix->size,
                                          0, node->size - 1, equal);
        }
        else {
            insertpos = search_insert_pos(node, newkey->addr, newkey->size, 0, node->size - 1, equal);
        }
#ifdef DUPKEY
        int rid = rand();
        vector<Key_c> allkeys;
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }
        char *newkey_ptr;
        if (head_comp) {
            newkey_ptr = new char[promotekey.size() - node->prefix.length() + 1];
            strcpy(newkey_ptr, promotekey.data() + node->prefix.length());
        }
        else {
            newkey_ptr = new char[promotekey.size() + 1];
            strcpy(newkey_ptr, promotekey.data());
        }
        allkeys.push_back(Key_c(newkey_ptr, rid));

        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }
#else
        // Insert promotekey into node->keys[insertpos]
        if (this->head_comp) {
            InsertKeyStd(node, insertpos, newkey->addr + node->prefix->size,
                         newkey->size - node->prefix->size);
        }
        else {
            InsertKeyStd(node, insertpos, newkey->addr, newkey->size);
        }

        // Insert the new childsplit.right into node->ptrs[insertpos + 1]
        InsertNode(node, insertpos + 1, childsplit->right);
#endif
    }
}

void BPTree::insert_leaf(Node *leaf, Node **path, int path_level, char *key, int keylen) {
    // TODO: modify check split to make sure the new key can always been inserted
    if (check_split_condition(leaf, keylen)) {
        splitReturn_new split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            Node *newRoot = new Node();
#ifdef DUPKEY
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(split.promotekey), rid));
            // TODO: to fix
#endif
            InsertNode(newRoot, 0, split.left);
            InsertKeyStd(newRoot, 0, split.promotekey.addr, split.promotekey.size);
            InsertNode(newRoot, 1, split.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            // nearest parent
            Node *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, &split);
        }
    }
    // Don't need to split
    else {
        int insertpos;
        bool equal = false;
        if (this->head_comp) {
            insertpos = search_insert_pos(leaf, key + leaf->prefix->size, keylen - leaf->prefix->size, 0,
                                          leaf->size - 1, equal);
        }
        else {
            insertpos = search_insert_pos(leaf, key, keylen, 0, leaf->size - 1, equal);
        }
#ifdef DUPKEY
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

            char *key_ptr = new char[strlen(key) + 1];
            if (this->head_comp) {
                strcpy(key_ptr, key + leaf->prefix.length());
            }
            else {
                strcpy(key_ptr, key);
            }

            allkeys.push_back(Key_c(key_ptr, rid));
            for (int i = insertpos; i < leaf->size; i++) {
                allkeys.push_back(leaf->keys.at(i));
            }
        }
        leaf->keys = allkeys;
#else
        // always insert no matter equal or not
        if (this->head_comp) {
            char *key_comp = key + leaf->prefix->size;
            InsertKeyStd(leaf, insertpos, key_comp, keylen - leaf->prefix->size);
        }
        else {
            InsertKeyStd(leaf, insertpos, key, keylen);
        }

#endif
    }
}

#ifdef DUPKEY
int BPTree::split_point(vector<Key_c> allkeys) {
    int size = allkeys.size();
    int bestsplit = size / 2;
    if (this->tail_comp) {
        int split_range_low = size / 3;
        int split_range_high = 2 * size / 3;
        // Representing 16 bytes of the integer
        int minlen = INT16_MAX;
        for (int i = split_range_low; i < split_range_high - 1; i++) {
            if (i + 1 < size) {
#ifdef DUPKEY
                string compressed =
                    tail_compress(allkeys.at(i).value, allkeys.at(i + 1).value);
#else
                string compressed = tail_compress(allkeys.at(i), allkeys.at(i + 1));
#endif
                if (compressed.length() < minlen) {
                    minlen = compressed.length();
                    bestsplit = i + 1;
                }
            }
        }
    }
    return bestsplit;
}

#else
int BPTree::split_point(Node *node) {
    int size = node->size;
    int bestsplit = size / 2;
    if (this->tail_comp) {
        int split_range_low = size * (1 / 2 - TAIL_SPLIT_WIDTH);
        int split_range_high = size * (1 / 2 + TAIL_SPLIT_WIDTH);
        // Representing 16 bytes of the integer
        int minlen = INT16_MAX;
        for (int i = split_range_low; i < split_range_high - 1; i++) {
            if (i + 1 < size) {
                Stdhead *h_i = GetHeaderStd(node, i);
                Stdhead *h_i1 = GetHeaderStd(node, i + 1);
                int cur_len = tail_compress_length(PageOffset(node, h_i->key_offset),
                                                   PageOffset(node, h_i1->key_offset),
                                                   h_i->key_len, h_i1->key_len);
                if (cur_len < minlen) {
                    minlen = cur_len;
                    bestsplit = i + 1;
                }
            }
        }
    }
    return bestsplit;
}
#endif

splitReturn_new BPTree::split_nonleaf(Node *node, int pos, splitReturn_new *childsplit) {
    splitReturn_new newsplit;
    const char *newkey = childsplit->promotekey.addr;
    uint8_t newkey_len = childsplit->promotekey.size;
    int insertpos;
    bool equal = false;

    if (this->head_comp) {
        insertpos = search_insert_pos(node, newkey + node->prefix->size,
                                      newkey_len - node->prefix->size,
                                      0, node->size - 1, equal);
    }
    else {
        insertpos = search_insert_pos(node, newkey, newkey_len, 0, node->size - 1, equal);
    }

    // Non-leaf nodes won't have duplicates,
    // so always insert newkey into the page for split
    // The promotekey has already been compressed
    if (this->head_comp) {
        InsertKeyStd(node, insertpos, newkey + node->prefix->size, newkey_len - node->prefix->size);
    }
    else {
        InsertKeyStd(node, insertpos, newkey, newkey_len);
    }

    // Insert the new right node to its parent
    InsertNode(node, insertpos + 1, childsplit->right);

    int split = split_point(node);

    Stdhead *head_fr = GetHeaderStd(node, split);
    char *firstright = PageOffset(node, head_fr->key_offset);
    int pkey_len;
    char *pkey_buf;
    if (this->head_comp && node->prefix->size) {
        pkey_len = node->prefix->size + head_fr->key_len;
        pkey_buf = new char[pkey_len + 1];
        strncpy(pkey_buf, node->prefix->addr, node->prefix->size);
        strcpy(pkey_buf + node->prefix->size, firstright);
    }
    else {
        pkey_len = head_fr->key_len;
        pkey_buf = new char[pkey_len + 1];
        strcpy(pkey_buf, firstright);
    }
    newsplit.promotekey.addr = pkey_buf;
    newsplit.promotekey.size = pkey_len;
    newsplit.promotekey.newallocated = true;

    // Copy the two parts into new pages
    Node *right = new Node();
    char *left_base = NewPage();
    char *right_base = right->base;
    SetEmptyPage(left_base);
    uint16_t left_top = 0;

    if (this->head_comp && pos >= 0) {
        // when pos < 0, it means we are spliting the root

        uint8_t leftprefix_len =
            head_compression_find_prefix_length(node->lowkey, &(newsplit.promotekey));
        uint8_t rightprefix_len =
            head_compression_find_prefix_length(&(newsplit.promotekey), node->highkey);

        CopyToNewPageStd(node, 0, split, left_base,
                         leftprefix_len - node->prefix->size, left_top);

        if (strcmp(node->highkey->addr, MAXHIGHKEY) != 0) {
            CopyToNewPageStd(node, split + 1, node->size, right->base,
                             rightprefix_len - node->prefix->size, right->space_top);

            char *newpfx = new char[rightprefix_len + 1];
            strncpy(newpfx, newsplit.promotekey.addr, rightprefix_len);
            newpfx[rightprefix_len] = '\0';
            UpdatePfxItem(right, newpfx, rightprefix_len, true);
        }
        else {
            // Assign the previous prefix to the new right node
            if (node->prefix->size > 0) {
                char *newpfx = new char[node->prefix->size + 1];
                strcpy(newpfx, node->prefix->addr);
                UpdatePfxItem(right, newpfx, node->prefix->size, true);
            }
            CopyToNewPageStd(node, split + 1, node->size, right->base, 0, right->space_top);
        }
        // Update left node, change early may change right_node udpate
        char *newpfx = new char[leftprefix_len + 1];
        strncpy(newpfx, node->lowkey->addr, leftprefix_len);
        newpfx[leftprefix_len] = '\0';
        UpdatePfxItem(node, newpfx, leftprefix_len, true);
    }
    else {
        // split + 1: the two parent are like: |p-k-p|  |p-k-p-k-p|, no need to
        // insert a k between them
        CopyToNewPageStd(node, 0, split, left_base, 0, left_top);
        CopyToNewPageStd(node, split + 1, node->size, right->base, 0, right->space_top);
    }

    // Separate the ptrs
    vector<Node *> leftptrs;
    vector<Node *> rightptrs;

    copy(node->ptrs.begin(), node->ptrs.begin() + split + 1,
         back_inserter(leftptrs));
    copy(node->ptrs.begin() + split + 1, node->ptrs.end(),
         back_inserter(rightptrs));

    right->size = node->size - (split + 1);
    right->IS_LEAF = false;

    right->ptrs = rightptrs;
    right->ptr_cnt = node->size - split;

    node->size = split;
    node->space_top = left_top;
    UpdateBase(node, left_base);

    node->ptrs = leftptrs;
    node->ptr_cnt = split + 1;

    // set key bound
    right->highkey = new WTitem(*node->highkey);
    node->highkey = new WTitem(newsplit.promotekey);
    right->lowkey = new WTitem(newsplit.promotekey);

    // Set next pointers
    Node *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturn_new BPTree::split_leaf(Node *node, char *newkey, int newkey_len) {
    splitReturn_new newsplit;
    Node *right = new Node();
    int insertpos;
    bool equal = false;
    if (this->head_comp) {
        insertpos = search_insert_pos(node, newkey + node->prefix->size, newkey_len - node->prefix->size, 0,
                                      node->size - 1, equal);
    }
    else {
        insertpos = search_insert_pos(node, newkey, newkey_len, 0, node->size - 1, equal);
    }

    // insert the new key into the page for split
    if (!equal) {
        if (this->head_comp) {
            char *key_comp = newkey + node->prefix->size;
            InsertKeyStd(node, insertpos, key_comp, newkey_len - node->prefix->size);
        }
        else {
            InsertKeyStd(node, insertpos, newkey, newkey_len);
        }
    }
    else {
        // just skip
    }

    int split = split_point(node);

    // calculate the separator
    Stdhead *head_fr = GetHeaderStd(node, split);
    char *firstright = PageOffset(node, head_fr->key_offset);
    char *s;
    int s_len;
    if (this->tail_comp) {
        Stdhead *head_ll = GetHeaderStd(node, split - 1);
        char *lastleft = PageOffset(node, head_ll->key_offset);

        s_len = tail_compress_length(lastleft, firstright,
                                     head_ll->key_len, head_fr->key_len);
        if (this->head_comp && node->prefix->size) {
            int pfxlen = node->prefix->size;
            s = new char[s_len + pfxlen + 1];
            strncpy(s, node->prefix->addr, pfxlen);
            strncpy(s + pfxlen, firstright, s_len);
            s_len += pfxlen;
        }
        else {
            s = new char[s_len + 1];
            strncpy(s, firstright, s_len);
        }
        s[s_len] = '\0';
    }
    else {
        if (this->head_comp && node->prefix->size) {
            s_len = node->prefix->size + head_fr->key_len;
            s = new char[s_len + 1];
            strncpy(s, node->prefix->addr, node->prefix->size);
            strcpy(s + node->prefix->size, firstright);
        }
        else {
            s_len = head_fr->key_len;
            s = new char[s_len + 1];
            strcpy(s, firstright);
        }
    }
    newsplit.promotekey.addr = s;
    newsplit.promotekey.size = s_len;
    newsplit.promotekey.newallocated = true;

    // Copy the two parts into new pages
    char *left_base = NewPage();
    char *right_base = right->base;
    SetEmptyPage(left_base);
    uint16_t left_top = 0;

    if (this->head_comp) {
        uint8_t leftprefix_len =
            head_compression_find_prefix_length(node->lowkey, &(newsplit.promotekey));
        uint8_t rightprefix_len =
            head_compression_find_prefix_length(&(newsplit.promotekey), node->highkey);

        CopyToNewPageStd(node, 0, split, left_base,
                         leftprefix_len - node->prefix->size, left_top);
        CopyToNewPageStd(node, split, node->size, right->base,
                         rightprefix_len - node->prefix->size, right->space_top);

        char *l_pfx = new char[leftprefix_len + 1];
        strncpy(l_pfx, node->lowkey->addr, leftprefix_len);
        l_pfx[leftprefix_len] = '\0';
        char *r_pfx = new char[rightprefix_len + 1];
        strncpy(r_pfx, newsplit.promotekey.addr, rightprefix_len);
        r_pfx[rightprefix_len] = '\0';

        UpdatePfxItem(node, l_pfx, leftprefix_len, true);
        UpdatePfxItem(right, r_pfx, rightprefix_len, true);
    }
    else {
        CopyToNewPageStd(node, 0, split, left_base, 0, left_top);
        CopyToNewPageStd(node, split, node->size, right->base, 0, right->space_top);
    }
    right->size = node->size - split;
    right->IS_LEAF = true;

    node->size = split;
    node->space_top = left_top;
    UpdateBase(node, left_base);

    // set key bound
    right->highkey = new WTitem(*node->highkey);
    node->highkey = new WTitem(newsplit.promotekey);
    right->lowkey = new WTitem(newsplit.promotekey);

    // Set next pointers
    Node *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTree::check_split_condition(Node *node, int keylen) {
    // split if the allocated page is full
    // double the key size to split safely
    // only works when the S_newkey <= S_prevkey + S_limit
    int currspace = node->space_top + node->size * sizeof(Stdhead);
    int splitcost = 2 * keylen + sizeof(Stdhead);
    if (currspace + splitcost >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    else
        return false;
}

int BPTree::search_insert_pos(Node *cursor, const char *key, int keylen, int low, int high,
                              bool &equal) {
    while (low <= high) {
        int mid = low + (high - low) / 2;

        Stdhead *header = GetHeaderStd(cursor, mid);
        int cmp = char_cmp_new(key, PageOffset(cursor, header->key_offset),
                               keylen, header->key_len);
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

Node *BPTree::search_leaf_node(Node *searchroot, const char *key, int keylen) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    Node *cursor = searchroot;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        int pos;
        if (this->head_comp) {
            pos = search_in_node(cursor, key + cursor->prefix->size, keylen - cursor->prefix->size, 0,
                                 cursor->size - 1, false);
        }
        else {
            pos = search_in_node(cursor, key, keylen, 0, cursor->size - 1, false);
        }
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

Node *BPTree::search_leaf_node_for_insert(Node *searchroot, const char *key, int keylen,
                                          Node **path, int &path_level) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    Node *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        path[path_level++] = cursor;
        int pos;
        if (this->head_comp) {
            pos = search_insert_pos(cursor, key + cursor->prefix->size, keylen - cursor->prefix->size, 0,
                                    cursor->size - 1, equal);
        }
        else {
            pos = search_insert_pos(cursor, key, keylen, 0, cursor->size - 1, equal);
        }
        cursor = cursor->ptrs[pos];
    }

    return cursor;
}

// TODO:merge these search function
int BPTree::search_in_node(Node *cursor, const char *key, int keylen,
                           int low, int high, bool isleaf) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Stdhead *header = GetHeaderStd(cursor, mid);
        int cmp = char_cmp_new(key, PageOffset(cursor, header->key_offset),
                               keylen, header->key_len);
        if (cmp == 0)
            return isleaf ? mid : mid + 1;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return isleaf ? -1 : high + 1;
}

/*
================================================
=============statistic function & printer=======
================================================
*/

void BPTree::getSize(Node *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                     int &totalBranching, unsigned long &totalKeySize,
                     int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
#ifdef DUPKEY
        for (int i = 0; i < cursor->size; i++) {
            currSize += cursor->keys.at(i).getSize();
        }
#else
        // subtract the \0
        // currSize += cursor->memusage - cursor->size;
        for (int i = 0; i < cursor->size; i++) {
            Stdhead *header = GetHeaderStd(cursor, i);
            currSize += header->key_len + sizeof(Stdhead);
        }
#endif
        totalKeySize += currSize + cursor->prefix->size;
        numKeys += cursor->size;
        totalPrefixSize += cursor->prefix->size;
        numNodes += 1;
        if (!cursor->IS_LEAF) {
            totalBranching += cursor->ptr_cnt;
            numNonLeaf += 1;
            for (int i = 0; i < cursor->size + 1; i++) {
                getSize(cursor->ptrs[i], numNodes, numNonLeaf, numKeys, totalBranching,
                        totalKeySize, totalPrefixSize);
            }
        }
    }
}

int BPTree::getHeight(Node *cursor) {
    if (cursor == NULL)
        return 0;
    if (cursor->IS_LEAF == true)
        return 1;
    int maxHeight = 0;
    for (int i = 0; i < cursor->size + 1; i++) {
        maxHeight = max(maxHeight, getHeight(cursor->ptrs[i]));
    }
    return maxHeight + 1;
}

void BPTree::printTree(Node *x, vector<bool> flag, bool compressed, int depth,
                       bool isLast) {
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
    // node is the rootnode
    if (depth == 0) {
        printKeys(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast) {
        cout << "+--- ";
        printKeys(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }
    else {
        cout << "+--- ";
        printKeys(x, compressed);
        cout << endl;
    }

    int it = 0;
    for (auto i = 0; i < x->ptr_cnt; i++)
        // Recursive call for the
        // children nodes
        printTree(x->ptrs[i], flag, compressed, depth + 1, i == x->ptr_cnt - 1);
    flag[depth] = true;
}