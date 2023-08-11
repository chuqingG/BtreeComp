#include "btree_myisam.h"
#include "./compression/compression_myisam.cpp"


// Initialise the BPTreeMyISAM NodeMyISAM
BPTreeMyISAM::BPTreeMyISAM(bool non_leaf_compression){
    root = NULL;
    non_leaf_comp = non_leaf_compression;
}

// Destructor of BPTreeMyISAM tree
BPTreeMyISAM::~BPTreeMyISAM(){
    delete root;
}

// Function to get the root NodeMyISAM
NodeMyISAM *BPTreeMyISAM::getRoot(){
    return root;
}

// Function to find any element
// in B+ Tree
int BPTreeMyISAM::search(string_view key){
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, key, parents);
    if (leaf == nullptr)
        return -1;
    return prefix_search(leaf, key);
}

void BPTreeMyISAM::insert(string x){
    if (root == nullptr){
        root = new NodeMyISAM;
        int rid = rand();
        root->keys.push_back(KeyMyISAM(x, 0, rid));
        root->IS_LEAF = true;
        root->size = 1;
        return;
    }
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, x, parents);
    insert_leaf(leaf, parents, x);
}

// Function to peform range query on B+Tree of MyISAM
int BPTreeMyISAM::searchRange(string min, string max){
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, min, parents);
    int pos = leaf != nullptr ?  prefix_search(leaf, min) : -1;
    int entries = 0;
    string prevkey = min;
    // Keep searching till value > max or we reach end of tree
    while (leaf != nullptr) {
        if (pos == leaf->size) {
            pos = 0;
            leaf = leaf->next;
            if (leaf == nullptr)
            {
                break;
            }
            prevkey = leaf->keys.at(0).value;
        }

        string leafkey = prevkey.substr(0, leaf->keys.at(pos).getPrefix()) + leaf->keys.at(pos).value;
        if (lex_compare(leafkey, max) > 0)
        {
            break;
        }
        prevkey = leafkey;
        entries += leaf->keys.at(pos).ridList.size();
        pos++;
    }
    return entries;
}

// Function to peform backward scan on B+tree
// Backward scan first decompresses the node in the forward direction
// and then scans in the backward direction. This is done to avoid overhead
// of decompression in the backward direction.
int BPTreeMyISAM::backwardScan(string min, string max) {
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, max, parents);
    int pos = leaf != nullptr ?  prefix_search(leaf, max) : -1;
    int entries = 0;
    vector<string> decompressed_keys = decompress_keys(leaf, pos);
    // Keep searching till value > min or we reach end of tree
    while (leaf != nullptr)
    {
        if (decompressed_keys.size() == 0)
        {
            leaf = leaf->prev;
            if (leaf == nullptr)
            {
                break;
            }
            decompressed_keys = decompress_keys(leaf);
            pos = decompressed_keys.size() - 1;
        }
        string leafkey = decompressed_keys.back();
        if (lex_compare(leafkey, min) < 0)
        {
            break;
        }
        entries += leaf->keys.at(pos).ridList.size();
        decompressed_keys.pop_back();
        pos--;
    }
    return entries;
}

//Decompress key in forward direction until a certain provided index
vector<string> BPTreeMyISAM::decompress_keys(NodeMyISAM *node, int pos) {
    vector<string> decompressed_keys;
    if (node == nullptr)
        return decompressed_keys;
    int size = pos == -1 ? node->keys.size() - 1 : pos; 
    string prevkey = "";
    for(int i = 0; i <= size; i++) {
        string key = prevkey.substr(0, node->keys.at(i).getPrefix()) + node->keys.at(i).value;
        decompressed_keys.push_back(key);
        prevkey = key;
    }
    return decompressed_keys;
}


/* Adapted from MyISAM code base
   Based on the implementation of _mi_prefix_search in
   {https://github.com/mysql/mysql-server/blob/a246bad76b9271cb4333634e954040a970222e0a/storage/myisam/mi_search.cc#L277}
*/
int BPTreeMyISAM::prefix_search(NodeMyISAM *cursor, string_view key){
    int cmplen;
    int len, matched;
    int my_flag;

    int ind = 0;
    cmplen = key.length();

    matched = 0; /* how many chars from prefix were already matched */
    len = 0;     /* length of previous key unpacked */

    while (ind < cursor->size){
        int prefix = cursor->keys.at(ind).getPrefix();
        string suffix = cursor->keys.at(ind).value;
        len = prefix + suffix.length();

        string pagekey = suffix;

        if (matched >= prefix){
            /* We have to compare. But we can still skip part of the key */
            uint left;

            /*
            If prefix_len > cmplen then we are in the end-space comparison
            phase. Do not try to access the key any more ==> left= 0.
            */
            left =
                ((len <= cmplen) ? suffix.length()
                                 : ((prefix < cmplen) ? cmplen - prefix : 0));

            matched = prefix + left;

            int ppos = 0;
            int kpos = prefix;
            for (my_flag = 0; left; left--){
                string c1(1, pagekey.at(ppos));
                string c2(1, key.at(kpos));
                if ((my_flag = c1.compare(c2)))
                    break;
                kpos++;
                ppos++;
            }
            if (my_flag > 0) /* mismatch */
                break;
            if (my_flag == 0){
                if (len < cmplen){
                    my_flag = -1;
                }else if (len > cmplen){
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

int BPTreeMyISAM::prefix_insert(NodeMyISAM *cursor, string_view key, bool &equal){
    int cmplen;
    int len, matched;
    int my_flag;

    int ind = 0;
    cmplen = key.length();
    matched = 0; /* how many chars from prefix were already matched */
    len = 0;     /* length of previous key unpacked */

    while (ind < cursor->size){
        int prefix = cursor->keys.at(ind).getPrefix();
        string suffix = cursor->keys.at(ind).value;
        len = prefix + suffix.length();

        string pagekey = suffix;

        if (matched >= prefix){
            /* We have to compare. But we can still skip part of the key */
            uint left;
            /*
            If prefix_len > cmplen then we are in the end-space comparison
            phase. Do not try to access the key any more ==> left= 0.
            */
            left =
                ((len <= cmplen) ? suffix.length()
                                 : ((prefix < cmplen) ? cmplen - prefix : 0));

            matched = prefix + left;

            int ppos = 0;
            int kpos = prefix;
            for (my_flag = 0; left; left--){
                string c1(1, pagekey.at(ppos));
                string c2(1, key.at(kpos));
                if ((my_flag = c1.compare(c2)))
                    break;
                ppos++;
                kpos++;
            }

            if (my_flag > 0) /* mismatch */
                return ind;
            if (my_flag == 0){
                if (len < cmplen){
                    my_flag = -1;
                }else if (len > cmplen){
                    my_flag = 1;
                }
                if (my_flag > 0)
                    return ind;
                if (my_flag == 0) {
                    equal = true;
                    return ind + 1;
                }
            }
            matched -= left;
        }

        ind++;
    }
    return cursor->size;
}

void BPTreeMyISAM::insert_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, int pos, splitReturnMyISAM childsplit){
    if (check_split_condition(node)){
        NodeMyISAM *parent = nullptr;
        if (pos >= 0){
            parent = parents.at(pos);
        }
        splitReturnMyISAM currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            NodeMyISAM *newRoot = new NodeMyISAM;
            int rid = rand();
            newRoot->keys.push_back(KeyMyISAM(currsplit.promotekey, 0, rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            root = newRoot;
        }else{
            if (parent == nullptr)
                return;
            insert_nonleaf(parent, parents, pos - 1, currsplit);
        }
    } else {
        string promotekey = childsplit.promotekey;
        int insertpos;
        bool equal = false;
        int rid = rand();
        if (this->non_leaf_comp) {
            insertpos = prefix_insert(node, promotekey, equal);
        } else {
            insertpos = insert_binary(node, promotekey, 0, node->size - 1, equal);
        }
        vector<KeyMyISAM> allkeys;
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        if(this->non_leaf_comp){
            if (insertpos > 0){
                string prev_key = get_key(node, insertpos - 1);
                int pfx = compute_prefix_myisam(prev_key, promotekey);
                string compressed = promotekey.substr(pfx);
                allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
            }else{
                allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
            }
        }else{
            allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
        }

        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(node->keys.at(i));
        }

        vector<NodeMyISAM *> allptrs;
        for (int i = 0; i < insertpos + 1; i++){
            allptrs.push_back(node->ptrs.at(i));
        }
        allptrs.push_back(childsplit.right);
        for (int i = insertpos + 1; i < node->size + 1; i++){
            allptrs.push_back(node->ptrs.at(i));
        }
        node->keys = allkeys;
        node->ptrs = allptrs;
        node->size = node->size + 1;

        if(this->non_leaf_comp)
            build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
    }
}

void BPTreeMyISAM::insert_leaf(NodeMyISAM *leaf, vector<NodeMyISAM *> &parents, string key){
    if (check_split_condition(leaf)){
        splitReturnMyISAM split = split_leaf(leaf, parents, key);
        if (leaf == root){
            NodeMyISAM *newRoot = new NodeMyISAM;
            int rid = rand();
            newRoot->keys.push_back(KeyMyISAM(split.promotekey, 0, rid));
            newRoot->ptrs.push_back(split.left);
            newRoot->ptrs.push_back(split.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            root = newRoot;
        }else{
            NodeMyISAM *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }else{
        int insertpos;
        int rid = rand();
        bool equal = false;
        insertpos = prefix_insert(leaf, key, equal);
        vector<KeyMyISAM> allkeys;
        if (equal) {
            allkeys = leaf->keys;
            allkeys.at(insertpos - 1).addRecord(rid);
        }
        else {
            for (int i = 0; i < insertpos; i++){
                allkeys.push_back(leaf->keys.at(i));
            }
            if (insertpos > 0){
                string prev_key = get_key(leaf, insertpos - 1);
                int pfx = compute_prefix_myisam(prev_key, key);
                string compressed = key.substr(pfx);
                allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
            }else{
                allkeys.push_back(KeyMyISAM(key, 0, rid));
            }

            for (int i = insertpos; i < leaf->size; i++){
                allkeys.push_back(leaf->keys.at(i));
            }
        }
        leaf->keys = allkeys;
        if (!equal) {
            leaf->size = leaf->size + 1;
            build_page_prefixes(leaf, insertpos, key); // Populate new prefixes
        }
    }
}

int BPTreeMyISAM::split_point(vector<KeyMyISAM> allkeys){
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnMyISAM BPTreeMyISAM::split_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> parents, int pos, splitReturnMyISAM childsplit){
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;
    string promotekey = childsplit.promotekey;
    int insertpos;
    string splitprefix;
    int rid = rand();
    bool equal = false;
    if (this->non_leaf_comp) {
        insertpos = prefix_insert(node, promotekey, equal);
    } else {
        insertpos = insert_binary(node, promotekey, 0, node->size - 1, equal);
    }

    vector<KeyMyISAM> allkeys;
    vector<NodeMyISAM *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }
    if (this->non_leaf_comp){
        if (insertpos > 0){
            string prev_key = get_key(node, insertpos - 1);
            int pfx = compute_prefix_myisam(prev_key, promotekey);
            string compressed = promotekey.substr(pfx);
            allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
        } else{
            allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
        }
    } else {
        allkeys.push_back(KeyMyISAM(promotekey, 0, rid));
    }

    for (int i = insertpos; i < node->size; i++){
        allkeys.push_back(node->keys.at(i));
    }

    for (int i = 0; i < insertpos + 1; i++){
        allptrs.push_back(node->ptrs.at(i));
    }
    allptrs.push_back(childsplit.right);
    for (int i = insertpos + 1; i < node->size + 1; i++){
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

    if(this->non_leaf_comp){
        firstright = get_uncompressed_key_before_insert(node, split + 1, insertpos, promotekey, equal);
        rightkeys.at(0) = KeyMyISAM(firstright, 0, rightkeys.at(0).ridList);
        splitkey = get_uncompressed_key_before_insert(node, split, insertpos, promotekey, equal);
    } else {
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

    if(this->non_leaf_comp){
        if (insertpos < split){
            build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
        }
        build_page_prefixes(right, 0, firstright); // Populate new prefixes
    }

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnMyISAM BPTreeMyISAM::split_leaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, string newkey){
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
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }

        if (insertpos > 0){
            string prev_key = get_key(node, insertpos - 1);
            int pfx = compute_prefix_myisam(prev_key, newkey);
            string compressed = newkey.substr(pfx);
            allkeys.push_back(KeyMyISAM(compressed, pfx, rid));
        } else{
            allkeys.push_back(KeyMyISAM(newkey, 0, rid));
        }
        
        for (int i = insertpos; i < node->size; i++){
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

    if (!equal && insertpos < split){
        build_page_prefixes(node, insertpos, newkey); // Populate new prefixes
    }
    build_page_prefixes(right, 0, firstright); // Populate new prefixes

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeMyISAM::check_split_condition(NodeMyISAM *node){
#ifdef SPLIT_STRATEGY_SPACE
    int currspace = 0;
    for (uint32_t i = 0; i < node->keys.size(); i++){
        currspace += node->keys.at(i).getSize();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeMyISAM::insert_binary(NodeMyISAM *cursor, string_view key, int low, int high, bool &equal){
    while (low <= high){
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

NodeMyISAM* BPTreeMyISAM::search_leaf_node(NodeMyISAM *root, string_view key, vector<NodeMyISAM *> &parents)
{
    // Tree is empty
    if (root == NULL){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeMyISAM *cursor = root;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF){
        string_view searchkey = key;
        parents.push_back(cursor);
        int pos;
        pos = prefix_insert(cursor, searchkey, equal);
        cursor = cursor->ptrs.at(pos);
    }
    return cursor;
}

int BPTreeMyISAM::search_binary(NodeMyISAM *cursor, string_view key, int low, int high){
    while (low <= high){
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
                     int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize){
    if (cursor != NULL){
        int currSize = 0;
        int prefixSize = 0;
        for (int i = 0; i < cursor->size; i++){
          currSize += cursor->keys.at(i).getSize();
          prefixSize += cursor->keys.at(i).getPrefix() <= 127 ? 1 : 2;
        }
        totalKeySize += currSize;
        numKeys += cursor->size;
        totalPrefixSize += prefixSize;
        numNodes += 1;
        if (cursor->IS_LEAF != true)
        {
          totalBranching += cursor->ptrs.size();
          numNonLeaf += 1;
          for (int i = 0; i < cursor->size + 1; i++){
            getSize(cursor->ptrs[i], numNodes, numNonLeaf, numKeys, totalBranching, totalKeySize, totalPrefixSize);
          }
        }
    }
}

int BPTreeMyISAM::getHeight(NodeMyISAM *cursor){
    if (cursor == NULL)
        return 0;
    if (cursor->IS_LEAF == true)
        return 1;
    int maxHeight = 0;
    for (int i = 0; i < cursor->size + 1; i++){
        maxHeight = max(maxHeight, getHeight(cursor->ptrs.at(i)));
    }
    return maxHeight + 1;
}

void BPTreeMyISAM::printTree(NodeMyISAM *x, vector<bool> flag, bool compressed, int depth, bool isLast)
{
    // Condition when node is None
    if (x == NULL)
        return;

    // Loop to print the depths of the
    // current node
    for (int i = 1; i < depth; ++i){

        // Condition when the depth
        // is exploring
        if (flag[i] == true){
            cout << "| " << " " << " " << " ";
        }

        // Otherwise print
        // the blank spaces
        else{
            cout << " " << " " << " " << " ";
        }
    }

    // Condition when the current
    // node is the root node
    if (depth == 0){
        printKeys_myisam(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast){
        cout << "+--- ";
        printKeys_myisam(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }else{
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