#include "btree_disk_pkb.h"

// Initialise the BPTreePkB NodePkB
BPTreePkB::BPTreePkB() {
    max_level = 1;

    dsk = new DskManager("file_pkb.txt");
    _root = new NodePkB();
    // _root = dsk->get_new_leaf_pkb();
    // _root->write_page(dsk->fp);
}

void deletefrom(NodePkB *node) {
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
// Destructor of BPTreePkB tree
BPTreePkB::~BPTreePkB() {
    deletefrom(_root);
    delete dsk;
}

// Function to get the root NodePkB
NodePkB *BPTreePkB::getRoot() {
    return _root;
}

// Function to find any element
// in B+ Tree
int BPTreePkB::search(const char *key) {
    int keylen = strlen(key);

    int offset = 0;
    bool equal = false;
    NodePkB *leaf = search_leaf_node(_root, key, keylen, offset);
    if (leaf == nullptr)
        return -1;
    // leaf->fetch_page(dsk->fp);
    findNodeResult result = find_node(leaf, key, keylen, offset, equal, dsk);
    // leaf->delete_from_mem();
    if (result.low == result.high)
        return result.low;
    else
        return -1;
}

int BPTreePkB::searchRange(const char *kmin, const char *kmax) {
    int min_len = strlen(kmin);
    int max_len = strlen(kmax);
    int offset = 0;
    NodePkB *leaf = search_leaf_node(_root, kmin, min_len, offset);
    if (leaf == nullptr)
        return 0;

    // leaf->fetch_page(dsk->fp);
    bool equal = false;
    findNodeResult result = find_node(leaf, kmin, min_len, offset, equal, dsk);
    int pos = -1;
    if (result.low == result.high)
        pos = result.low;

    int entries = 0;
    // Keep searching till value > max or we reach end of tree
    while (1) {
        if (pos == leaf->size) {
            // leaf->delete_from_mem();
            pos = 0;
            leaf = leaf->next;
            if (leaf == nullptr)
                return entries;
            // leaf->fetch_page(dsk->fp);
            continue;
        }

        PkBhead *head = GetHeaderPkB(leaf, pos);
        char *buf = new char[head->key_len + 1];
        GetKeyInDisk(buf, head->key_offset, dsk, head->key_len);
        // if (char_cmp_new(PageOffset(leaf, head->key_offset), kmax,
        //                  head->key_len, max_len)
        if (char_cmp_new(buf, kmax, head->key_len, max_len) > 0) {
            break;
        }
        delete[] buf;
        entries++;
        pos++;
    }
    // leaf->delete_from_mem();
    return entries;
}

void BPTreePkB::insert(char *key) {
    int keylen = strlen(key);

    int offset = 0;
    int path_level = 0;
    NodePkB *search_path[max_level];
    NodePkB *leaf = search_leaf_node_for_insert(_root, key, keylen,
                                                search_path, path_level, offset);
    insert_leaf(leaf, search_path, path_level, key, keylen, offset);
}

void BPTreePkB::insert_nonleaf(NodePkB *node, NodePkB **path, int parentlevel, splitReturnPkB *childsplit) {
    // int offset;
    Item basekey;
    get_base_key_from_ancestor(path, parentlevel, node, basekey, dsk->fp);
    int offset = get_common_prefix_len(basekey.addr, childsplit->promotekey.addr,
                                       basekey.size, childsplit->promotekey.size);

    if (check_split_condition(node, childsplit->promotekey.size)) {
        NodePkB *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturnPkB currsplit = split_nonleaf(node, path, parentlevel, childsplit, offset);
        if (node == _root) {
            NodePkB *newRoot = new NodePkB;

            InsertNode(newRoot, 0, currsplit.left);
            InsertKeyPkB(dsk, newRoot, 0,
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

        bool equal = false;
        findNodeResult result = find_node(node, promotekey->addr, promotekey->size,
                                          offset, equal, dsk);
        int insertpos = result.high;

        Item prevkey;
        generate_pkb_key(node, promotekey->addr, promotekey->size, insertpos,
                         path, parentlevel, prevkey, dsk->fp);
        int pfx_len = get_common_prefix_len(prevkey.addr, promotekey->addr, prevkey.size, promotekey->size);
        InsertKeyPkB(dsk, node, insertpos, promotekey->addr, promotekey->size, pfx_len);

        update_next_prefix(node, insertpos, promotekey->addr, promotekey->size, dsk->fp);

        InsertNode(node, insertpos + 1, childsplit->right);
    }
}

void BPTreePkB::insert_leaf(NodePkB *leaf, NodePkB **path, int path_level,
                            char *key, int keylen, int offset) {
    if (check_split_condition(leaf, keylen)) {
        splitReturnPkB split = split_leaf(leaf, path, path_level, key, keylen, offset);
        if (leaf == _root) {
            NodePkB *newRoot = new NodePkB;

            InsertNode(newRoot, 0, split.left);
            InsertKeyPkB(dsk, newRoot, 0,
                         split.promotekey.addr, split.promotekey.size, 0);
            InsertNode(newRoot, 1, split.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            NodePkB *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, &split);
        }
    }
    else {
        bool equal = false;
        // fetch the page content back
        // leaf->fetch_page(dsk->fp);

        findNodeResult result = find_node(leaf, key, keylen, offset, equal, dsk);
        int insertpos = result.high;

        Item prevkey;
        generate_pkb_key(leaf, key, keylen, insertpos,
                         path, path_level - 1, prevkey, dsk->fp);
        int pfx_len = get_common_prefix_len(prevkey.addr, key, prevkey.size, keylen);
        InsertKeyPkB(dsk, leaf, insertpos, key, keylen, pfx_len);

        if (!equal) {
            update_next_prefix(leaf, insertpos, key, keylen, dsk->fp);
        }
        // Write back to disk, delete the copy in memory
        // leaf->write_page(dsk->fp);
    }
}

int BPTreePkB::split_point(NodePkB *node) {
    int bestsplit = node->size / 2;
    return bestsplit;
}

splitReturnPkB BPTreePkB::split_nonleaf(NodePkB *node, NodePkB **path, int parentlevel,
                                        splitReturnPkB *childsplit, int offset) {
    splitReturnPkB newsplit;
    NodePkB *right = new NodePkB;
    Item *promotekey = &(childsplit->promotekey);

    bool equal = false;
    findNodeResult result = find_node(node, promotekey->addr, promotekey->size,
                                      offset, equal, dsk);
    int insertpos = result.high;

    // Insert the new key
    Item prevkey;
    generate_pkb_key(node, promotekey->addr, promotekey->size, insertpos,
                     path, parentlevel, prevkey, dsk->fp);
    int pfx_len = get_common_prefix_len(prevkey.addr, promotekey->addr, prevkey.size, promotekey->size);
    InsertKeyPkB(dsk, node, insertpos, promotekey->addr, promotekey->size, pfx_len);
    if (!equal) {
        update_next_prefix(node, insertpos, promotekey->addr, promotekey->size, dsk->fp);
    }

    InsertNode(node, insertpos + 1, childsplit->right);

    int split = split_point(node);

    // 1. get the full separator
    PkBhead *head_split = GetHeaderPkB(node, split);
    newsplit.promotekey.size = head_split->key_len;
    newsplit.promotekey.addr = new char[newsplit.promotekey.size + 1];
    GetKeyInDisk(newsplit.promotekey.addr, head_split->key_offset, dsk, head_split->key_len);
    // strcpy(newsplit.promotekey.addr, PageOffset(node, head_split->key_offset));
    newsplit.promotekey.newallocated = true;

    // 2. fetch the full firstright(pos + 1), and compute pfx_len
    // update the firstright key
    // Or no need to update, cause it's originally set
    // Difference with split_leaf: the separator is indeed the firstright, when here are different
    // PkBhead *head_rf = GetHeaderPkB(node, split + 1);
    // int pfx_len_rf = get_common_prefix_len(newsplit.promotekey.addr, PageOffset(node, head_rf->key_offset),
    //                                        newsplit.promotekey.size, head_rf->key_len);

    // 3. copy the two parts into new pages
    char *leftbase = NewPage();
    SetEmptyPage(leftbase);
    char *rightbase = right->base;
    // int left_top = 0, right_top = 0;

    CopyToNewPagePkB(node, 0, split, leftbase);
    CopyToNewPagePkB(node, split + 1, node->size, rightbase);

    // 4. update pointers
    vector<NodePkB *> leftptrs;
    vector<NodePkB *> rightptrs;
    copy(node->ptrs.begin(), node->ptrs.begin() + split + 1,
         back_inserter(leftptrs));
    copy(node->ptrs.begin() + split + 1, node->ptrs.end(),
         back_inserter(rightptrs));

    right->size = node->size - (split + 1);
    // right->space_top = right_top;
    right->IS_LEAF = false;
    right->ptrs = rightptrs;
    right->ptr_cnt = node->size - split;

    node->size = split;
    // node->space_top = left_top;
    UpdateBase(node, leftbase);
    node->ptrs = leftptrs;
    node->ptr_cnt = split + 1;

    // Set next pointers
    NodePkB *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnPkB BPTreePkB::split_leaf(NodePkB *node, NodePkB **path, int path_level,
                                     char *newkey, int keylen, int offset) {
    splitReturnPkB newsplit;
    // NodePkB *right = dsk->get_new_leaf_pkb();
    NodePkB *right = new NodePkB();

    bool equal = false;
    // node->fetch_page(dsk->fp);

    findNodeResult result = find_node(node, newkey, keylen, offset, equal, dsk);
    int insertpos = result.high;

    // Insert the new key
    Item prevkey;
    generate_pkb_key(node, newkey, keylen, insertpos,
                     path, path_level - 1, prevkey, dsk->fp);
    int pfx_len = get_common_prefix_len(prevkey.addr, newkey, prevkey.size, keylen);
    InsertKeyPkB(dsk, node, insertpos, newkey, keylen, pfx_len);

    if (!equal) {
        update_next_prefix(node, insertpos, newkey, keylen, dsk->fp);
    }

    int split = split_point(node);

    // 1. get full promotekey, make a copy for that
    PkBhead *head_split = GetHeaderPkB(node, split);
    newsplit.promotekey.size = head_split->key_len;
    newsplit.promotekey.addr = new char[newsplit.promotekey.size + 1];
    GetKeyInDisk(newsplit.promotekey.addr, head_split->key_offset, dsk, head_split->key_len);
    // strcpy(newsplit.promotekey.addr, PageOffset(node, head_split->key_offset));
    newsplit.promotekey.newallocated = true;

    // 2. Set the pfx_len of firstright to its length, the base key is the ancester
    head_split->pfx_len = head_split->key_len;
    head_split->pk_len = 0;
    memset(head_split->pk, 0, sizeof(head_split->pk));

    // 3. copy the two parts into new pages
    char *leftbase = NewPage();
    SetEmptyPage(leftbase);
    char *rightbase = right->base;
    // int left_top = 0, right_top = 0;

    CopyToNewPagePkB(node, 0, split, leftbase);
    CopyToNewPagePkB(node, split, node->size, rightbase);

    right->size = node->size - split;
    // right->space_top = right_top;
    right->IS_LEAF = true;
    // right->write_page(dsk->fp);

    node->size = split;
    // node->space_top = left_top;
    UpdateBase(node, leftbase);
    // UpdateBaseInDisk(node, leftbase, dsk);

    // Set next pointers
    NodePkB *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreePkB::check_split_condition(NodePkB *node, int keylen) {
    int currspace = node->size * sizeof(PkBhead);
    // Update prefix need more space, the newkey,
    // should leave space for current insert and the following split
    int splitcost = sizeof(PkBhead);
    // cout << "cur, split:" << currspace << splitcost << endl;
    if (currspace + 2 * splitcost >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    else
        return false;
}

NodePkB *BPTreePkB::search_leaf_node(NodePkB *searchroot, const char *key, int keylen, int &offset) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodePkB *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        findNodeResult result = find_node(cursor, key, keylen, offset, equal, dsk);
        // If equal key found, choose next position
        int pos;
        if (result.low == result.high)
            pos = result.high + 1;
        else
            pos = result.high;
        offset = result.offset;
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

NodePkB *BPTreePkB::search_leaf_node_for_insert(NodePkB *searchroot, const char *key, int keylen,
                                                NodePkB **path, int &path_level, int &offset) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodePkB *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        path[path_level++] = cursor;

        findNodeResult result = find_node(cursor, key, keylen, offset, equal, dsk);
        // If equal key found, choose next position
        int pos;
        if (result.low == result.high)
            pos = result.high + 1;
        else
            pos = result.high;
        offset = result.offset;
        // if (pos >= cursor->ptr_cnt) {
        //     cout << "wrong index" << endl;
        // }
        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

/*
================================================
=============statistic function & printer=======
================================================
*/

void BPTreePkB::getSize(NodePkB *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                        int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize) {
    if (cursor != NULL) {
        unsigned long currSize = 0;
        int prefixSize = 0;
        // if (cursor->IS_LEAF)
        //     cursor->fetch_page(dsk->fp);

        for (int i = 0; i < cursor->size; i++) {
            PkBhead *head = GetHeaderPkB(cursor, i);
            currSize += head->key_len + sizeof(PkBhead);
            prefixSize += sizeof(head->pfx_len) + head->pk_len + sizeof(head->pfx_len);
        }
        // if (cursor->IS_LEAF)
        //     cursor->delete_from_mem();
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

int BPTreePkB::getHeight(NodePkB *cursor) {
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

void BPTreePkB::printTree(NodePkB *x, vector<bool> flag, bool compressed, int depth, bool isLast) {
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
        printKeys_pkb(x, compressed, dsk->fp);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast) {
        cout << "+--- ";
        printKeys_pkb(x, compressed, dsk->fp);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }
    else {
        cout << "+--- ";
        printKeys_pkb(x, compressed, dsk->fp);
        cout << endl;
    }

    for (auto i = 0; i < x->ptr_cnt; i++)
        // Recursive call for the
        // children nodes
        printTree(x->ptrs[i], flag, compressed, depth + 1,
                  i == (x->ptrs.size()) - 1);
    flag[depth] = true;
}