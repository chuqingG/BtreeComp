#include "btree_disk_db2.h"

// Initialise the BPTreeDB2 NodeDB2
BPTreeDB2::BPTreeDB2() {
    _root = nullptr;
    max_level = 1;

    dsk = new DskManager("file_db2.txt");
    _root = dsk->get_new_leaf_db2();
    _root->write_page(dsk->fp);
}

void deletefrom(NodeDB2 *node) {
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

// Destructor of BPTreeDB2 tree
BPTreeDB2::~BPTreeDB2() {
    deletefrom(_root);
    // delete _root;
}

// Function to get the root NodeDB2
NodeDB2 *BPTreeDB2::getRoot() {
    return _root;
}

// Function to find any element
// in B+ Tree
int BPTreeDB2::search(const char *key) {
    int keylen = strlen(key);
    NodeDB2 *leaf = search_leaf_node(_root, key, keylen);
    if (leaf == nullptr)
        return -1;
    int metadatapos = find_prefix_pos(leaf, key, keylen, false);
    if (metadatapos == -1)
        return -1;
    DB2pfxhead *pfx = GetHeaderDB2pfx(leaf, metadatapos);
    return search_in_leaf(leaf, key + pfx->pfx_len, keylen - pfx->pfx_len, pfx->low, pfx->high);
}

int BPTreeDB2::searchRange(const char *kmin, const char *kmax) {
    int min_len = strlen(kmin);
    int max_len = strlen(kmax);
    // vector<DB2Node *> parents;
    NodeDB2 *leaf = search_leaf_node(_root, kmin, min_len);
    int pos, metadatapos;
    if (leaf == nullptr) {
        return 0;
    }
    else {
        metadatapos = find_prefix_pos(leaf, kmin, min_len, false);
        if (metadatapos == -1) {
            return 0;
        }
        else {
            DB2pfxhead *pfx = GetHeaderDB2pfx(leaf, metadatapos);
            pos = search_in_leaf(leaf, kmin + pfx->pfx_len, min_len - pfx->pfx_len, pfx->low, pfx->high);
        }
    }
    int entries = 0;
    // Keep searching till value > max or we reach end of tree
    while (leaf != nullptr) {
        if (pos == leaf->size) {
            leaf = leaf->next;
            pos = 0;
            metadatapos = 0;
            continue;
        }

        DB2pfxhead *pfx = GetHeaderDB2pfx(leaf, metadatapos);

        int pfxcmp = strncmp(PfxOffset(leaf, pfx->pfx_offset), kmax, pfx->pfx_len);
        if (pfxcmp > 0)
            break;
        else if (pfxcmp < 0) {
            // Decompress all keys in this prefix
            for (int i = pos; i <= pfx->high; i++) {
                DB2head *head_i = GetHeaderDB2(leaf, i);
                char *decomp_key = new char[pfx->pfx_len + head_i->key_len + 1];
                strncpy(decomp_key, PfxOffset(leaf, pfx->pfx_offset), pfx->pfx_len);
                strcpy(decomp_key + pfx->pfx_len, PageOffset(leaf, head_i->key_offset));
                delete decomp_key;
            }
            entries += pfx->high - pos + 1;
            metadatapos++;
            pos = pfx->high + 1;
            continue;
        }
        int i;
        for (i = pos; i <= pfx->high; i++) {
            DB2head *head_i = GetHeaderDB2(leaf, i);
            if (char_cmp_new(PageOffset(leaf, head_i->key_offset), kmax + pfx->pfx_len,
                             head_i->key_len, max_len - pfx->pfx_len)
                > 0)
                break;
            char *decomp_key = new char[pfx->pfx_len + head_i->key_len + 1];
            strncpy(decomp_key, PfxOffset(leaf, pfx->pfx_offset), pfx->pfx_len);
            strcpy(decomp_key + pfx->pfx_len, PageOffset(leaf, head_i->key_offset));
            delete decomp_key;
            entries++;
        }
        if (i <= pfx->high)
            break;
        else {
            metadatapos++;
            pos = i;
        }
    }
    return entries;
}

void BPTreeDB2::insert(char *x) {
    int keylen = strlen(x);
    if (_root == nullptr) {
        _root = new NodeDB2;
        InsertKeyDB2(_root, 0, x, keylen);
        return;
    }
    NodeDB2 *search_path[max_level];
    int path_level = 0;
    NodeDB2 *leaf = search_leaf_node_for_insert(_root, x, keylen, search_path, path_level);
    insert_leaf(leaf, search_path, path_level, x, keylen);
}

void BPTreeDB2::insert_nonleaf(NodeDB2 *node, NodeDB2 **path,
                               int parentlevel, splitReturnDB2 *childsplit) {
    if (check_split_condition(node, childsplit->promotekey.size)) {
        apply_prefix_optimization(node);
    }
    if (check_split_condition(node, childsplit->promotekey.size)) {
        NodeDB2 *parent = nullptr;
        if (parentlevel >= 0) {
            parent = path[parentlevel];
        }
        splitReturnDB2 currsplit = split_nonleaf(node, parentlevel, childsplit);
        if (node == _root) {
            NodeDB2 *newRoot = new NodeDB2;

            InsertNode(newRoot, 0, currsplit.left);
            InsertKeyDB2(newRoot, 0, currsplit.promotekey.addr, currsplit.promotekey.size);
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
        // string promotekey = childsplit.promotekey;
        bool equal = false;
        int insertpos = insert_prefix_and_key(node,
                                              childsplit->promotekey.addr,
                                              childsplit->promotekey.size, equal);

        InsertNode(node, insertpos + 1, childsplit->right);
    }
}

void BPTreeDB2::insert_leaf(NodeDB2 *leaf, NodeDB2 **path, int path_level, char *key, int keylen) {
    if (check_split_condition(leaf, keylen)) {
        apply_prefix_optimization(leaf);
    }
    if (check_split_condition(leaf, keylen)) {
        splitReturnDB2 split = split_leaf(leaf, key, keylen);
        if (leaf == _root) {
            NodeDB2 *newRoot = new NodeDB2;
            InsertNode(newRoot, 0, split.left);
            InsertKeyDB2(newRoot, 0, split.promotekey.addr, split.promotekey.size);
            InsertNode(newRoot, 1, split.right);

            newRoot->IS_LEAF = false;
            _root = newRoot;
            max_level++;
        }
        else {
            NodeDB2 *parent = path[path_level - 1];
            insert_nonleaf(parent, path, path_level - 2, &split);
        }
    }
    else {
        int insertpos;
        bool equal = false;
        // The key was concated inside it
        insertpos = insert_prefix_and_key(leaf, key, keylen, equal);
    }
}

int BPTreeDB2::insert_prefix_and_key(NodeDB2 *node, const char *key, int keylen, bool &equal) {
    // Find a pos, then insert into it
    int pfx_pos = find_prefix_pos(node, key, keylen, true);
    int insertpos;
    if (pfx_pos == node->pfx_size) {
        // need to add new prefix
        insertpos = node->size;
        InsertPfxDB2(node, pfx_pos, "", 0, insertpos, insertpos);
    }
    else {
        DB2pfxhead *pfxhead = GetHeaderDB2pfx(node, pfx_pos);
        if (strncmp(key, PfxOffset(node, pfxhead->pfx_offset), pfxhead->pfx_len) == 0) {
            insertpos = search_insert_pos(node, key + pfxhead->pfx_len, keylen - pfxhead->pfx_len,
                                          pfxhead->low, pfxhead->high, equal);
            pfxhead->high += 1;
        }
        else {
            insertpos = pfxhead->low;
            InsertPfxDB2(node, pfx_pos, "", 0, insertpos, insertpos);
        }
        for (int i = pfx_pos + 1; i < node->pfx_size; i++) {
            DB2pfxhead *head_i = GetHeaderDB2pfx(node, i);
            head_i->low += 1;
            head_i->high += 1;
        }
    }
    DB2pfxhead *head = GetHeaderDB2pfx(node, pfx_pos);
    InsertKeyDB2(node, insertpos, key + head->pfx_len, keylen - head->pfx_len);

    return insertpos;
}

int BPTreeDB2::split_point(NodeDB2 *node) {
    int bestsplit = node->size / 2;
    return bestsplit;
}

void BPTreeDB2::do_split_node(NodeDB2 *node, NodeDB2 *right, int splitpos, bool isleaf, Item &splitprefix) {
    int rightindex = 0;

    char *l_base = NewPageDB2();
    SetEmptyPageDB2(l_base);

    uint16_t l_usage = 0;

    int pfx_top_l = 0;
    int pfx_size_l = 0;

    for (int i = 0; i < node->pfx_size; i++) {
        DB2pfxhead *pfx_i = GetHeaderDB2pfx(node, i);
        // split occurs within this pfx
        if (splitpos >= pfx_i->low && splitpos <= pfx_i->high) {
            splitprefix.addr = PfxOffset(node, pfx_i->pfx_offset);
            splitprefix.size = pfx_i->pfx_len;

            if (splitpos != pfx_i->low) {
                // Copy the prefix, set the new header
                strcpy(l_base + MAX_SIZE_IN_BYTES + pfx_top_l, PfxOffset(node, pfx_i->pfx_offset));
                DB2pfxhead *header = (DB2pfxhead *)(l_base + MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE
                                                    - (pfx_size_l + 1) * sizeof(DB2pfxhead));
                header->low = pfx_i->low;
                header->high = splitpos - 1;
                header->pfx_len = pfx_i->pfx_len;
                header->pfx_offset = pfx_top_l;
                pfx_top_l += pfx_i->pfx_len + 1;
                pfx_size_l++;
            }

            int rsize = isleaf ? pfx_i->high - splitpos : pfx_i->high - (splitpos + 1);
            if (rsize >= 0) {
                InsertPfxDB2(right, right->pfx_size,
                             PfxOffset(node, pfx_i->pfx_offset), pfx_i->pfx_len,
                             rightindex, rightindex + rsize);

                rightindex = rightindex + rsize + 1;
            }
        }
        // to the right
        else if (splitpos <= pfx_i->low) {
            int size = pfx_i->high - pfx_i->low;
            InsertPfxDB2(right, right->pfx_size,
                         PfxOffset(node, pfx_i->pfx_offset), pfx_i->pfx_len,
                         rightindex, rightindex + size);

            rightindex += size + 1;
        }
        // to the left
        else {
            strcpy(l_base + MAX_SIZE_IN_BYTES + pfx_top_l, PfxOffset(node, pfx_i->pfx_offset));
            DB2pfxhead *header = (DB2pfxhead *)(l_base + MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE
                                                - (pfx_size_l + 1) * sizeof(DB2pfxhead));

            memcpy(header, pfx_i, sizeof(DB2pfxhead));
            header->pfx_offset = pfx_top_l;
            pfx_top_l += pfx_i->pfx_len + 1;
            pfx_size_l++;
        }
    }
    // should make a copy of splitprefix before the l_pfx is reallocated
    char *newsplitpfx = new char[splitprefix.size + 1];
    strncpy(newsplitpfx, splitprefix.addr, splitprefix.size);
    newsplitpfx[splitprefix.size] = '\0';
    splitprefix.addr = newsplitpfx;
    splitprefix.newallocated = true;

    // update the prefix area of node
    node->pfx_size = pfx_size_l;
    node->pfx_top = pfx_top_l;

    // Split the keys to the new page

    CopyToNewPageDB2(node, 0, splitpos, l_base, l_usage);
    // CopyKeyToPage(node, 0, splitpos, l_base, l_usage, l_idx, l_size);
    if (isleaf) {
        CopyToNewPageDB2(node, splitpos, node->size, right->base, right->space_top);
        right->size = node->size - splitpos;
    }
    else {
        CopyToNewPageDB2(node, splitpos + 1, node->size, right->base, right->space_top);
        right->size = node->size - splitpos - 1;
    }
    right->IS_LEAF = isleaf;

    node->size = splitpos;
    node->space_top = l_usage;
    UpdateBase(node, l_base);
    // no need to update size

    // Set prev and next pointers
    NodeDB2 *next = node->next;
    node->next = right;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;

    return;
}

splitReturnDB2 BPTreeDB2::split_nonleaf(NodeDB2 *node, int pos, splitReturnDB2 *childsplit) {
    splitReturnDB2 newsplit;
    NodeDB2 *right = new NodeDB2;
    right->pfx_size = 0;

    bool equal = false;
    int insertpos = insert_prefix_and_key(node,
                                          childsplit->promotekey.addr,
                                          childsplit->promotekey.size, equal);
    InsertNode(node, insertpos + 1, childsplit->right);
    int split = split_point(node);

    DB2head *head_split = GetHeaderDB2(node, split);
    Item splitkey;
    splitkey.size = head_split->key_len;
    splitkey.addr = new char[splitkey.size + 1];
    splitkey.newallocated = true;
    strcpy(splitkey.addr, PageOffset(node, head_split->key_offset));

    Item split_prefix;
    do_split_node(node, right, split, false, split_prefix);

    vector<NodeDB2 *> leftptrs;

    copy(node->ptrs.begin(), node->ptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(node->ptrs.begin() + split + 1, node->ptrs.end(), back_inserter(right->ptrs));

    right->ptr_cnt = node->ptr_cnt - (split + 1);
    node->ptr_cnt = split + 1;
    node->ptrs = leftptrs;

    int split_len = split_prefix.size + splitkey.size;
    char *split_decomp = new char[split_len + 1];
    strncpy(split_decomp, split_prefix.addr, split_prefix.size);
    strcpy(split_decomp + split_prefix.size, splitkey.addr);

    newsplit.promotekey.addr = split_decomp;
    newsplit.promotekey.size = split_len;
    newsplit.promotekey.newallocated = true;
    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnDB2 BPTreeDB2::split_leaf(NodeDB2 *node, char *newkey, int keylen) {
    splitReturnDB2 newsplit;
    NodeDB2 *right = new NodeDB2;
    right->pfx_size = 0;
    bool equal = false;

    int insertpos = insert_prefix_and_key(node, newkey, keylen, equal);
    int split = split_point(node);

    Item splitkey;
    do_split_node(node, right, split, true, splitkey);

    DB2pfxhead *rf_pfx = GetHeaderDB2pfx(right, 0);
    DB2head *rf = GetHeaderDB2(right, 0);

    int rf_len = rf_pfx->pfx_len + rf->key_len;
    char *rightfirst = new char[rf_len + 1];
    strncpy(rightfirst, PfxOffset(right, rf_pfx->pfx_offset), rf_pfx->pfx_len);
    strcpy(rightfirst + rf_pfx->pfx_len, PageOffset(right, rf->key_offset));

    newsplit.promotekey.addr = rightfirst;
    newsplit.promotekey.size = rf_len;
    newsplit.promotekey.newallocated = true;
    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeDB2::check_split_condition(NodeDB2 *node, int keylen) {
    // Just simplify, may need to consider the prefix size
    int currspace = node->space_top + node->size * sizeof(DB2head);
    // copy from myisam
    int splitcost = keylen + sizeof(DB2head);
    int nextkey = sizeof(DB2head) + keylen;
    int currspace_pfx = node->pfx_top + node->pfx_size * sizeof(DB2pfxhead);
    int splitcost_pfx = sizeof(DB2pfxhead);     // must leave space for next split
    int optimcost_pfx = 2 * sizeof(DB2pfxhead); // for safety, prefix expand introduce new pfx segments

    if (currspace + splitcost + nextkey >= MAX_SIZE_IN_BYTES - SPLIT_LIMIT)
        return true;
    else if (currspace_pfx + splitcost_pfx + optimcost_pfx >= DB2_PFX_MAX_SIZE - SPLIT_LIMIT)
        return true;
    return false;
}

int BPTreeDB2::search_insert_pos(NodeDB2 *cursor, const char *key, int keylen,
                                 int low, int high, bool &equal) {
    // Search pos in node when insert, or search non-leaf node when search
    while (low <= high) {
        int mid = low + (high - low) / 2;
        DB2head *head = GetHeaderDB2(cursor, mid);
        int cmp = char_cmp_new(key, PageOffset(cursor, head->key_offset),
                               keylen, head->key_len);
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

NodeDB2 *BPTreeDB2::search_leaf_node(NodeDB2 *searchroot, const char *key, int keylen) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeDB2 *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        // string_view searchkey = key;
        int pos = -1;
        int metadatapos = find_prefix_pos(cursor, key, keylen, true);
        if (unlikely(metadatapos == cursor->pfx_size)) {
            pos = cursor->size;
        }
        else {
            DB2pfxhead *pfxhead = GetHeaderDB2pfx(cursor, metadatapos);

            int prefixcmp = memcmp(key, PfxOffset(cursor, pfxhead->pfx_offset), sizeof(char) * pfxhead->pfx_len);
            if (prefixcmp != 0) {
                pos = prefixcmp > 0 ? pfxhead->high + 1 : pfxhead->low;
            }
            else {
                pos = search_insert_pos(cursor, key + pfxhead->pfx_len, keylen - pfxhead->pfx_len,
                                        pfxhead->low, pfxhead->high, equal);
            }
        }
        cursor = cursor->ptrs.at(pos);
    }
    return cursor;
}

NodeDB2 *BPTreeDB2::search_leaf_node_for_insert(NodeDB2 *searchroot, const char *key, int keylen, NodeDB2 **path, int &path_level) {
    // Tree is empty
    if (searchroot == NULL) {
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeDB2 *cursor = searchroot;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        path[path_level++] = cursor;
        int pos = -1;
        int metadatapos = find_prefix_pos(cursor, key, keylen, true);
        if (metadatapos == cursor->pfx_size) {
            pos = cursor->size;
        }
        else {
            DB2pfxhead *pfxhead = GetHeaderDB2pfx(cursor, metadatapos);

            int prefixcmp = strncmp(key, PfxOffset(cursor, pfxhead->pfx_offset), pfxhead->pfx_len);
            if (prefixcmp != 0) {
                pos = prefixcmp > 0 ? pfxhead->high + 1 : pfxhead->low;
            }
            else {
                pos = search_insert_pos(cursor, key + pfxhead->pfx_len, keylen - pfxhead->pfx_len,
                                        pfxhead->low, pfxhead->high, equal);
            }
        }

        cursor = cursor->ptrs[pos];
    }
    return cursor;
}

int BPTreeDB2::search_in_leaf(NodeDB2 *cursor, const char *key, int keylen, int low, int high) {
    while (low <= high) {
        int mid = low + (high - low) / 2;
        DB2head *head = GetHeaderDB2(cursor, mid);
        int cmp = char_cmp_new(key, PageOffset(cursor, head->key_offset),
                               keylen, head->key_len);
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

void BPTreeDB2::getSize(NodeDB2 *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                        int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize) {
    if (cursor != NULL) {
        int currSize = 0;
        int prefixSize = 0;
        for (int i = 0; i < cursor->pfx_size; i++) {
            DB2pfxhead *head = GetHeaderDB2pfx(cursor, i);
            currSize += head->pfx_len + sizeof(DB2pfxhead);
            prefixSize += head->pfx_len + sizeof(DB2pfxhead);
        }
        for (int i = 0; i < cursor->size; i++) {
            DB2head *head = GetHeaderDB2(cursor, i);
            currSize += head->key_len + sizeof(DB2head);
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

int BPTreeDB2::getHeight(NodeDB2 *cursor) {
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

void BPTreeDB2::printTree(NodeDB2 *x, vector<bool> flag, bool compressed, int depth, bool isLast) {
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

    for (auto i = 0; i < x->ptr_cnt; i++)
        // Recursive call for the
        // children nodes
        printTree(x->ptrs[i], flag, compressed, depth + 1,
                  i == (x->ptrs.size()) - 1);
    flag[depth] = true;
}