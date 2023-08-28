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
        return search_in_leaf(leaf, key + leaf->prefix->size, keylen - leaf->prefix->size, 0, leaf->size - 1);
    }
    else {
        return search_in_leaf(leaf, key, keylen, 0, leaf->size - 1);
    }
}

void BPTree::insert(char *x) {
    int keylen = strlen(x);
    if (_root == nullptr) {
        _root = new Node(this->head_comp, this->tail_comp);
#ifdef DUPKEY
        int rid = rand();
        _root->keys.push_back(Key_c(x, rid));
        _root->size = 1;
#else
        InsertKey(_root, 0, x, keylen);
#endif
        _root->IS_LEAF = true;
        return;
    }
    
    Node* search_path[max_level];
    int path_level = 0;
    Node *leaf = search_leaf_node_for_insert(_root, x, keylen, search_path, path_level);
    insert_leaf(leaf, search_path, path_level, x, keylen);
    
}


void BPTree::insert_nonleaf(Node *node, Node **path,
                            int parentlevel, splitReturn_new childsplit) {
    if (check_split_condition(node, childsplit.promotekey->size)) {
        Node *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturn_new currsplit =
            split_nonleaf(node, parentlevel, childsplit);
        if (node == _root) {
            Node *newRoot = new Node(this->head_comp, this->tail_comp);
#ifdef DUPKEY
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(currsplit.promotekey), rid));
#endif
            InsertNode(newRoot, 0, currsplit.left);
            InsertKey(newRoot, 0, currsplit.promotekey->addr(), currsplit.promotekey->size);
            InsertNode(newRoot, 1, currsplit.right);
            
            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            if (parent == nullptr) {
                return;
            }
            insert_nonleaf(parent, path, parentlevel - 1, currsplit);
        }
    }
    else {
        Data *newkey = childsplit.promotekey;
        int insertpos;
        bool equal = false;
        if (this->head_comp) {
            // promotekey = promotekey.substr(node->prefix.length());
            insertpos = search_insert_pos(node, newkey->addr() + node->prefix->size, newkey->size - node->prefix->size, 0,
                                          node->size - 1, equal);
        }
        else {
            insertpos = search_insert_pos(node, newkey->addr(), newkey->size, 0, node->size - 1, equal);
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
            const char *key_comp = newkey->addr() + node->prefix->size;
            InsertKey(node, insertpos, key_comp, newkey->size - node->prefix->size);
        }
        else {
            InsertKey(node, insertpos, newkey->addr(), newkey->size);
        }

        // Insert the new childsplit.right into node->ptrs[insertpos + 1]
        InsertNode(node, insertpos + 1, childsplit.right);
#endif
    }
}

void BPTree::insert_leaf(Node *leaf, Node** path, int path_level, char *key, int keylen) {
    // TODO: modify check split to make sure the new key can always been inserted
    if (check_split_condition(leaf, keylen)) {
        splitReturn_new split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            Node *newRoot = new Node(this->head_comp, this->tail_comp);
#ifdef DUPKEY
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(split.promotekey), rid));
            // TODO: to fix
#endif
            InsertNode(newRoot, 0, split.left);
            InsertKey(newRoot, 0, split.promotekey->addr(), split.promotekey->size);
            InsertNode(newRoot, 1, split.right);
           
            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            // nearest parent
            Node *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, split);
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
        if (!equal) {
            if (this->head_comp) {
                char *key_comp = key + leaf->prefix->size;
                InsertKey(leaf, insertpos, key_comp, keylen - leaf->prefix->size);
            }
            else {
                InsertKey(leaf, insertpos, key, keylen);
            }
        }
        else {
            // just skip
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
        int split_range_low = size / 3;
        int split_range_high = 2 * size / 3;
        // Representing 16 bytes of the integer
        int minlen = INT16_MAX;
        for (int i = split_range_low; i < split_range_high - 1; i++) {
            if (i + 1 < size) {
                int cur_len = tail_compress_length(GetKey(node, i), GetKey(node, i + 1),
                                            node->keys_size[i], node->keys_size[i + 1]);
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

splitReturn_new BPTree::split_nonleaf(Node *node, int pos, splitReturn_new childsplit) {
    splitReturn_new newsplit;
    const char *newkey = childsplit.promotekey->addr();
    int newkey_len = childsplit.promotekey->size;
    int insertpos;
    bool equal = false;

    if (this->head_comp) {
        // promotekey = promotekey.substr(node->prefix.length());
        insertpos = search_insert_pos(node, newkey + node->prefix->size, newkey_len - node->prefix->size, 0,
                                      node->size - 1, equal);
    }
    else {
        insertpos = search_insert_pos(node, newkey, newkey_len, 0, node->size - 1, equal);
    }

    vector<Node *> allptrs;
#ifdef DUPKEY
    int rid = rand();
    vector<Key_c> allkeys;
    for (int i = 0; i < insertpos; i++) {
        allkeys.push_back(node->keys.at(i));
    }
    // Non-leaf nodes won't have duplicates
    //  insert new here, may need to create new to deallocate
    char *newkey_ptr = new char[promotekey.size() + 1];
    strcpy(newkey_ptr, promotekey.data());
    allkeys.push_back(Key_c(newkey_ptr, rid));
    // allkeys.push_back(Key(promotekey, rid));

    for (int i = insertpos; i < node->size; i++) {
        allkeys.push_back(node->keys.at(i));
    }
    int split = split_point(allkeys);

    vector<Key_c> leftkeys;
    vector<Key_c> rightkeys;
#else
    // Non-leaf nodes won't have duplicates,
    // so always insert newkey into the page for split
    // The promotekey has already been compressed
    if (this->head_comp) {
        InsertKey(node, insertpos, newkey + node->prefix->size, newkey_len - node->prefix->size);
    }
    else {
        InsertKey(node, insertpos, newkey, newkey_len);
    }

    // Insert the new right node to its parent
    InsertNode(node, insertpos + 1, childsplit.right);

    int split = split_point(node);

    // cout << "Best Split point " << split << " size " << allkeys.size() << endl;

    char *firstright = GetKey(node, split);
    int pkey_len = strlen(firstright);
    char *pkey_buf;
    if (this->head_comp && node->prefix->size) {
        pkey_len += node->prefix->size;
        pkey_buf = new char[pkey_len + 1];
        sprintf(pkey_buf, "%s%s", node->prefix->addr(), firstright);
        newsplit.promotekey = new Data(pkey_buf, pkey_len);
        // newsplit.promotekey = node->prefix.addr() + firstright;
    }
    else {
        pkey_buf = new char[pkey_len + 1];
        strcpy(pkey_buf, firstright);
        newsplit.promotekey = new Data(pkey_buf, pkey_len);
        // newsplit.promotekey = firstright;
    }

    // Copy the two parts into new pages
    // TODO: directly use the space of new right page
    uint16_t* left_idx = new uint16_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    uint16_t* right_idx = new uint16_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    uint8_t* left_size = new uint8_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    uint8_t* right_size = new uint8_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    char *left_base;
    char *right_base;
    int left_mem = 0;
    int right_mem = 0;

    Node *right = new Node(this->head_comp, this->tail_comp);
    left_base = NewPage();
    right_base = right->base;

    if (this->head_comp && pos >= 0) {
        // when pos < 0, it means we are spliting the root

        int leftprefix_len =
            head_compression_find_prefix_length(node->lowkey, newsplit.promotekey);
        int rightprefix_len =
            head_compression_find_prefix_length(newsplit.promotekey, node->highkey);

        left_mem = update_page_prefix(node, left_base, left_idx, left_size,
                                      leftprefix_len, 0, split);

        if (strcmp(node->highkey->addr(), MAXHIGHKEY) != 0) {
            right_mem = update_page_prefix(node, right_base, right_idx, right_size,
                                           rightprefix_len, split + 1, node->size);

            right->prefix = new Data(newsplit.promotekey->addr(), rightprefix_len);
        }
        else {
            // Assign the previous prefix to the new right node
            right->prefix = new Data(node->prefix->addr(), node->prefix->size);
            right_mem = update_page_prefix(node, right_base, right_idx, right_size,
                                           node->prefix->size, split + 1, node->size);
        }
        // Update left node, change early may change right_node udpate
        node->prefix = new Data(node->lowkey->addr(),leftprefix_len);
    }
    else {
        // split + 1: the two parent are like: |p-k-p|  |p-k-p-k-p|, no need to
        // insert a k between them
        CopyKeyToPage(node, 0, split, left_base, left_mem, left_idx, left_size);
        CopyKeyToPage(node, split + 1, node->size, right_base, right_mem,
                      right_idx, right_size);
    }

    // Separate the ptrs
    vector<Node *> leftptrs;
    vector<Node *> rightptrs;
    // Node* leftptrs[kNumberBound + 1];
    // Node** rightptrs =new Node*[kNumberBound + 1];
    
    copy(node->ptrs.begin(), node->ptrs.begin() + split + 1,
         back_inserter(leftptrs));
    copy(node->ptrs.begin() + split + 1, node->ptrs.end(),
         back_inserter(rightptrs));
    
    // for(int i = 0; i < split + 1; i++)
    //     leftptrs[i] = node->ptrs[i];
    // // For convenience, should be split+1 -> size+1
    // for(int i = split; i < node->size; i++)
    //     rightptrs[i - split] = node->ptrs[i + 1];
    
    right->size = node->size - (split + 1);
    right->IS_LEAF = false;
    right->memusage = right_mem;
    right->keys_offset = right_idx;
    right->keys_size = right_size;
    right->ptrs = rightptrs;
    right->ptr_cnt = node->size - split;

    node->size = split;
    node->memusage = left_mem;
    UpdateOffset(node, left_idx);
    UpdatePtrs(node, leftptrs, split + 1);
    UpdateBase(node, left_base);
    UpdateSize(node, left_size);

    // set key bound
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;
    right->lowkey = newsplit.promotekey;

    // Set next pointers
    Node *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;
#endif
    return newsplit;
}

splitReturn_new BPTree::split_leaf(Node *node, char *newkey, int newkey_len) {
    splitReturn_new newsplit;
    Node *right = new Node(this->head_comp, this->tail_comp);
    int insertpos;
    bool equal = false;
    if (this->head_comp) {
        insertpos = search_insert_pos(node, newkey + node->prefix->size, newkey_len - node->prefix->size, 0,
                                      node->size - 1, equal);
    }
    else {
        insertpos = search_insert_pos(node, newkey, newkey_len, 0, node->size - 1, equal);
    }

#ifdef DUPKEY
    int rid = rand();
    vector<Key_c> allkeys;
    if (equal) {
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++) {
            allkeys.push_back(node->keys.at(i));
        }
        // insert new here, may need to create new to deallocate
        char *newkey_ptr = new char[strlen(newkey) + 2];
        if (this->head_comp) {
            strcpy(newkey_ptr, newkey + node->prefix.length());
        }
        else {
            strcpy(newkey_ptr, newkey);
        }
        allkeys.push_back(Key_c(newkey_ptr, rid));

        for (int i = insertpos; i < node->size; i++) {
            allkeys.push_back(node->keys.at(i));
        }
    }
    int split = split_point(allkeys);

    vector<Key_c> leftkeys;
    vector<Key_c> rightkeys;

    // TODO:need to add corresponding code
#else
    // insert the new key into the page for split
    if (!equal) {
        if (this->head_comp) {
            char *key_comp = newkey + node->prefix->size;
            InsertKey(node, insertpos, key_comp, newkey_len - node->prefix->size);
        }
        else {
            InsertKey(node, insertpos, newkey, newkey_len);
        }
    }
    else {
        // just skip
    }

    int split = split_point(node);

    // calculate the separator
    if (this->tail_comp) {
        char* lastleft = GetKey(node, split - 1);
        char* firstright = GetKey(node, split);
        int s_len = tail_compress_length(lastleft, firstright, 
                            node->keys_size[split - 1], node->keys_size[split]);
        char *s;
        if (this->head_comp) {
            int pfxlen = node->prefix->size;
            s = new char[s_len + pfxlen + 1];
            strncpy(s, node->prefix->addr(), pfxlen);
            strncpy(s + pfxlen, firstright, s_len);
            s_len += pfxlen;
        }else{
            s = new char[s_len + 1];
            strncpy(s, firstright, s_len);
        }
        s[s_len] = '\0';

        // string s = tail_compress(lastleft, firstright);

        newsplit.promotekey = new Data(s, s_len);
    }
    else {
        char *firstright = GetKey(node, split);
        int pkey_len = strlen(firstright);
        if (this->head_comp && node->prefix->size) {
            pkey_len += node->prefix->size;
            char *s = new char[pkey_len + 1];
            sprintf(s, "%s%s", node->prefix->addr(), firstright);
            newsplit.promotekey = new Data(s, pkey_len);
            // newsplit.promotekey = node->prefix.addr() + firstright;
        }
        else {
            char *s = new char[pkey_len + 1];
            strcpy(s, firstright);
            newsplit.promotekey = new Data(s, pkey_len);
        }
    }

    // Copy the two parts into new pages
    // vector<uint16_t> left_idx(split);
    // vector<uint16_t> right_idx(node->size - split);
    // vector<uint16_t> left_idx;
    // vector<uint16_t> right_idx;
    uint16_t* left_idx = new uint16_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    uint16_t* right_idx = new uint16_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    uint8_t* left_size = new uint8_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    uint8_t* right_size = new uint8_t[kNumberBound * (1 + this->head_comp * HEAD_COMP_RELAX + this->tail_comp * TAIL_COMP_RELAX)];
    char *left_base;
    char *right_base;
    int left_mem = 0;
    int right_mem = 0;

    left_base = NewPage();
    right_base = right->base;
    //TODO: fix keys_size for head comp
    if (this->head_comp) {
        
        int leftprefix_len =
            head_compression_find_prefix_length(node->lowkey, newsplit.promotekey);
        int rightprefix_len =
            head_compression_find_prefix_length(newsplit.promotekey, node->highkey);

        left_mem = update_page_prefix(node, left_base, left_idx, left_size,
                                      leftprefix_len, 0, split);
        right_mem = update_page_prefix(node, right_base, right_idx, right_size,
                                       rightprefix_len, split, node->size);

        node->prefix = new Data(node->lowkey->addr(), leftprefix_len);
        right->prefix = new Data(newsplit.promotekey->addr(), rightprefix_len);
    }
    else {
        CopyKeyToPage(node, 0, split, left_base, left_mem, left_idx, left_size);
        CopyKeyToPage(node, split, node->size, right_base, right_mem, right_idx, right_size);
    }

    // rewrite left page to origin node, set the new right node

    right->size = node->size - split;
    right->memusage = right_mem;
    right->keys_offset = right_idx;
    right->keys_size = right_size;
    right->IS_LEAF = true;

    node->size = split;
    node->memusage = left_mem;
    UpdateOffset(node, left_idx);
    UpdateBase(node, left_base);
    UpdateSize(node, left_size)

    // set key bound
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;
    right->lowkey = newsplit.promotekey;

    // Set next pointers
    Node *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;
#endif
    return newsplit;
}

#ifdef DUPKEY
bool BPTree::check_split_condition(Node *node) {
#ifdef SPLIT_STRATEGY_SPACE
    int currspace = node->prefix.length();
    for (uint32_t i = 0; i < node->keys_offset.size(); i++) {
        currspace += node->keys.at(i).getSize();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}
#else

bool BPTree::check_split_condition(Node *node, int keylen) {
    // split if the allocated page is full
    // double the key size to split safely
    // only works when the S_newkey <= S_prevkey + S_limit
    if(node->memusage + 2 * keylen >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    if(this->tail_comp && !node->IS_LEAF && node->size > kNumberBound * MAX_TAIL_COMP_RATIO - 2)
        return true;
    return false;
    // return (node->memusage + 2 * strlen(key) >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT);
}
#endif

int BPTree::search_insert_pos(Node *cursor, const char *key, int keylen, int low, int high,
                              bool &equal) {
    // char* page = cursor->base;
    while (low <= high) {
        int mid = low + (high - low) / 2;
#ifdef DUPKEY
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
#else
        int cmp = char_cmp(key, GetKey(cursor, mid), keylen);
#endif
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
        // string_view searchkey = key;
        int pos;
        if (this->head_comp) {
            pos = search_in_nonleaf(cursor, key + cursor->prefix->size, keylen - cursor->prefix->size, 0,
                                    cursor->size - 1);
        }
        else {
            pos = search_in_nonleaf(cursor, key, keylen, 0, cursor->size - 1);
        }
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

Node *BPTree::search_leaf_node_for_insert(Node *searchroot, const char *key, int keylen,
                                          Node ** path, int &path_level) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    Node *cursor = searchroot;
    bool equal = false;
    // int path_level = 0;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        // string_view searchkey = key;
        // parents.push_back(cursor);
        path[path_level++] = cursor;
        int pos;
        if (this->head_comp) {
            pos = search_insert_pos(cursor, key + cursor->prefix->size, keylen - cursor->prefix->size, 0,
                                    cursor->size - 1, equal);
        }
        else {
            pos = search_insert_pos(cursor, key, keylen, 0, cursor->size - 1, equal);
        }
        // searchkey = key.substr(cursor->prefix.length());
        // pos = search_insert_pos(cursor, searchkey, 0, cursor->size - 1, equal);
        cursor = cursor->ptrs[pos];
    }

    return cursor;
}

int BPTree::search_in_nonleaf(Node *cursor, const char *key, int keylen, int low,
                              int high) {
    // char* page = cursor->base;
    while (low <= high) {
        int mid = low + (high - low) / 2;
#ifdef DUPKEY
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
#else
        int cmp = char_cmp(key, GetKey(cursor, mid), keylen);
#endif
        if (cmp == 0) {
            return mid + 1;
        }
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return high + 1;
}

int BPTree::search_in_leaf(Node *cursor, const char *key, int keylen, int low, int high) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
#ifdef DUPKEY
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
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

int BPTree::get_child_pos_in_parent(Node *parent, Node *node) {
    for (uint32_t i = 0; i < parent->ptr_cnt; i++) {
        if (parent->ptrs[i] == node)
            return i;
    }
    return -1;
}

#ifdef DUPKEY
void BPTree::get_node_bounds(vector<Node *> parents, int pos, Node *node,
                             string &lower_bound, string &upper_bound) {
    Node *curr = node;
    bool lower_set = false;
    bool upper_set = false;
    for (int i = pos; i >= 0; i--) {
        if (lower_set && upper_set)
            return;
        Node *parent = parents.at(i);
        int nodepos = get_child_pos_in_parent(parent, curr);
        int size = parent->size;
        if (nodepos > 0 && nodepos < size) {
            if (!lower_set) {
                lower_bound = parent->prefix + parent->keys[nodepos - 1].value;
                lower_set = true;
            }
            if (!upper_set) {
                upper_bound = parent->prefix + parent->keys[nodepos].value;
                upper_set = true;
            }
        }
        else if (nodepos == 0) {
            if (!upper_set) {
                upper_bound = parent->prefix + parent->keys[0].value;
                upper_set = true;
            }
        }
        else {
            if (!lower_set) {
                lower_bound = parent->prefix + parent->keys[size - 1].value;
                lower_set = true;
            }
        }
        curr = parent;
    }
}
#else
void BPTree::get_node_bounds(vector<Node *> parents, int pos, Node *node,
                             string &lower_bound, string &upper_bound) {
    Node *curr = node;
    bool lower_set = false;
    bool upper_set = false;
    for (int i = pos; i >= 0; i--) {
        if (lower_set && upper_set)
            return;
        Node *parent = parents.at(i);
        int nodepos = get_child_pos_in_parent(parent, curr);
        int size = parent->size;

        if (nodepos > 0 && nodepos < size) {
            if (!lower_set) {
                lower_bound = parent->prefix->addr();
                lower_bound += GetKey(parent, nodepos - 1);
                lower_set = true;
            }
            if (!upper_set) {
                upper_bound = parent->prefix->addr();
                upper_bound += GetKey(parent, nodepos);
                upper_set = true;
            }
        }
        else if (nodepos == 0) {
            if (!upper_set) {
                upper_bound = parent->prefix->addr();
                upper_bound += GetKey(parent, 0);
                upper_set = true;
            }
        }
        else {
            if (!lower_set) {
                lower_bound = parent->prefix->addr();
                lower_bound += GetKey(parent, size - 1);
                lower_set = true;
            }
        }
        curr = parent;
    }
}
#endif

int BPTree::update_page_prefix(Node *node, char *page, uint16_t *idx, uint8_t *lens,
                               int prefixlen, int low, int high) {
    // Compress node[low:high) to *page, return memusage
    uint16_t memusage = 0;
    // The new prefix must be longer, it's no need to use the original prefix
    int cutoff = prefixlen - node->prefix->size;
    for (int i = low; i < high; i++) {
        char *k = GetKey(node, i) + cutoff;
        int klen = strlen(k);
        strcpy(page + memusage, k);
        idx[i - low] = memusage;
        lens[i - low] = klen;
        memusage += klen + 1;
    }
    return memusage;
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
        currSize += cursor->memusage - cursor->size;
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