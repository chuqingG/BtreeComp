#include "btree_wt.h"
#include "./compression/compression_wt.cpp"


// Initialise the BPTreeWT NodeWT
BPTreeWT::BPTreeWT(bool non_leaf_compression,
                bool suffix_compression){
    root = NULL;
    non_leaf_comp = non_leaf_compression;
    suffix_comp = suffix_compression;
}

// Destructor of BPTreeWT tree
BPTreeWT::~BPTreeWT(){
    delete root;
}

// Function to get the root NodeWT
NodeWT *BPTreeWT::getRoot(){
    return root;
}

// Function to find any element
// in B+ Tree
int BPTreeWT::search(string_view key){
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, key, parents, skiplow);
    if (leaf == nullptr)
        return -1;
    return search_binary(leaf, key, 0, leaf->size - 1, skiplow);
}

void BPTreeWT::insert(string x){
    if (root == nullptr){
        root = new NodeWT;
        int rid = rand();
        root->keys.push_back(KeyWT(x, 0, rid));
        root->IS_LEAF = true;
        root->size = 1;
        return;
    }
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, x, parents, skiplow);
    insert_leaf(leaf, parents, x);
}

// Function to peform range query on B+Tree of WT
int BPTreeWT::searchRange(string min, string max){
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, min, parents, skiplow);
    int pos =  leaf != nullptr ?  search_binary(leaf, min, 0, leaf->size - 1, skiplow) : -1;
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

        string leafkey = prevkey.substr(0, leaf->keys.at(pos).prefix) + leaf->keys.at(pos).value;
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
int BPTreeWT::backwardScan(string min, string max) {
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, max, parents, skiplow);
    int pos =  leaf != nullptr ?  search_binary(leaf, max, 0, leaf->size - 1, skiplow) : -1;
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
vector<string> BPTreeWT::decompress_keys(NodeWT *node, int pos) {
    vector<string> decompressed_keys;
    if (node == nullptr)
        return decompressed_keys;
    int size = pos == -1 ? node->keys.size() - 1 : pos; 
    string prevkey = "";
    for(int i = 0; i <= size; i++) {
        string key = prevkey.substr(0, node->keys.at(i).prefix) + node->keys.at(i).value;
        decompressed_keys.push_back(key);
        prevkey = key;
    }
    return decompressed_keys;
}

void BPTreeWT::insert_nonleaf(NodeWT *node, vector<NodeWT *> &parents, int pos, splitReturnWT childsplit){
    if (check_split_condition(node)){
        NodeWT *parent = nullptr;
        if (pos >= 0){
            parent = parents.at(pos);
        }
        splitReturnWT currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            NodeWT *newRoot = new NodeWT;
            int rid = rand();
            newRoot->keys.push_back(KeyWT(currsplit.promotekey, 0, rid));
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
        size_t skiplow = 0;
        int rid = rand();
        bool equal = false;
        int insertpos = insert_binary(node, promotekey, 0, node->size - 1, skiplow, equal);
        vector<KeyWT> allkeys;
        vector<NodeWT *> allptrs;
        
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }

        if(this->non_leaf_comp){
            if (insertpos > 0){
                string prev_key = get_key(node, insertpos - 1);
                uint8_t pfx = compute_prefix_wt(prev_key, promotekey);
                string compressed = promotekey.substr(pfx);
                allkeys.push_back(KeyWT(compressed, pfx, rid));
            }else{
                allkeys.push_back(KeyWT(promotekey, 0, rid));
            }
        } else {
            allkeys.push_back(KeyWT(promotekey, 0, rid));
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
        node->keys = allkeys;
        node->ptrs = allptrs;
        node->size = node->size + 1;

        if(this->non_leaf_comp){
            // Rebuild prefixes if a new item is inserted in position 0
            build_page_prefixes(node, insertpos, promotekey); // Populate new prefixes
        }
    }
}

void BPTreeWT::insert_leaf(NodeWT *leaf, vector<NodeWT *> &parents, string key){
    if (check_split_condition(leaf)){
        splitReturnWT split = split_leaf(leaf, parents, key);
        if (leaf == root){
            NodeWT *newRoot = new NodeWT;
            int rid = rand();
            newRoot->keys.push_back(KeyWT(split.promotekey, 0, rid));
            newRoot->ptrs.push_back(split.left);
            newRoot->ptrs.push_back(split.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            root = newRoot;
        }else{
            NodeWT *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }else{
        size_t skiplow = 0;
        int rid = rand();
        bool equal = false;
        int insertpos = insert_binary(leaf, key, 0, leaf->size - 1, skiplow, equal);
        
        vector<KeyWT> allkeys;
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
                uint8_t pfx = compute_prefix_wt(prev_key, key);
                string compressed = key.substr(pfx);
                allkeys.push_back(KeyWT(compressed, pfx, rid));
            } else {
                allkeys.push_back(KeyWT(key, 0, rid));
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

int BPTreeWT::split_point(vector<KeyWT> allkeys){
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnWT BPTreeWT::split_nonleaf(NodeWT *node, vector<NodeWT *> parents, int pos, splitReturnWT childsplit){
    splitReturnWT newsplit;
    NodeWT *right = new NodeWT;
    string promotekey = childsplit.promotekey;
    string splitprefix;
    size_t skiplow = 0;
    int rid = rand();
    bool equal = false;
    int insertpos = insert_binary(node, promotekey, 0, node->size - 1, skiplow, equal);
    
    vector<KeyWT> allkeys;
    vector<NodeWT *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
    }

    //Non-leaf nodes won't have duplicates
    if(this->non_leaf_comp){
        if (insertpos > 0){
            string prev_key = get_key(node, insertpos - 1);
            uint8_t pfx = compute_prefix_wt(prev_key, promotekey);
            string compressed = promotekey.substr(pfx);
            allkeys.push_back(KeyWT(compressed, pfx, rid));
        } else {
            allkeys.push_back(KeyWT(promotekey, 0, rid));
        }
    } else {
        allkeys.push_back(KeyWT(promotekey, 0, rid));
    }
    
    for (int i = insertpos; i < node->size; i++){
        allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
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

    vector<KeyWT> leftkeys;
    vector<KeyWT> rightkeys;
    vector<NodeWT *> leftptrs;
    vector<NodeWT *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    string splitkey;
    string firstright;

    if(this->non_leaf_comp){
        firstright = get_uncompressed_key_before_insert(node, split + 1, insertpos, promotekey, equal);
        rightkeys.at(0) = KeyWT(firstright, 0, rightkeys.at(0).ridList);
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
    NodeWT *next = node->next;
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

splitReturnWT BPTreeWT::split_leaf(NodeWT *node, vector<NodeWT *> &parents, string newkey){
    splitReturnWT newsplit;
    NodeWT *right = new NodeWT;
    size_t skiplow = 0;
    int rid = rand();
    bool equal = false;
    int insertpos = insert_binary(node, newkey, 0, node->size - 1, skiplow, equal);
    vector<KeyWT> allkeys;

    if (equal) {
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
        }
        if (insertpos > 0){
            string prev_key = get_key(node, insertpos - 1);
            uint8_t pfx = compute_prefix_wt(prev_key, newkey);
            string compressed = newkey.substr(pfx);
            allkeys.push_back(KeyWT(compressed, pfx, rid));
        } else {
            allkeys.push_back(KeyWT(newkey, 0, rid));
        }

        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(KeyWT(node->keys.at(i).value, node->keys.at(i).prefix, node->keys.at(i).ridList));
        }

    }

    int split = split_point(allkeys);

    vector<KeyWT> leftkeys;
    vector<KeyWT> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));
    int orgind;
    string firstright;

    firstright = get_uncompressed_key_before_insert(node, split, insertpos, newkey, equal);
    rightkeys.at(0) = KeyWT(firstright, 0, rightkeys.at(0).ridList);

    if(this->suffix_comp){
        string lastleft = get_uncompressed_key_before_insert(node, split - 1, insertpos, newkey, equal);
        newsplit.promotekey = promote_key(node, lastleft, firstright);
    } else {
        newsplit.promotekey = firstright;
    }

    node->size = leftkeys.size();
    node->keys = leftkeys;

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;

    if (!equal && insertpos < split){
        build_page_prefixes(node, insertpos, newkey); // Populate new prefixes
    }
    build_page_prefixes(right, 0, firstright); // Populate new prefixes

    // Set next pointers
    NodeWT *next = node->next;
    right->prev = node;
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeWT::check_split_condition(NodeWT *node){
#ifdef SPLIT_STRATEGY_SPACE
    int currspace = 0;
    for (int i = 0; i < node->keys.size(); i++){
        currspace += node->keys.at(i).getSize();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeWT::insert_binary(NodeWT *cursor, string_view key, int low, int high, size_t &skiplow, bool &equal){
    size_t skiphigh = 0;
    while (low <= high){
        int mid = low + (high - low) / 2;
        string pagekey = extract_key(cursor, mid, this->non_leaf_comp);
        int cmp;
        size_t match = min(skiplow, skiphigh);
        cmp = lex_compare_skip(string(key), pagekey, &match);
        if (cmp > 0){
            skiplow = match;
        } else if (cmp < 0){
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

NodeWT* BPTreeWT::search_leaf_node(NodeWT *root, string_view key, vector<NodeWT *> &parents, size_t &skiplow)
{
    // Tree is empty
    if (root == NULL){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeWT *cursor = root;
    bool equal = false;
    // Till we reach leaf node
    while (!cursor->IS_LEAF){
        string_view searchkey = key;
        parents.push_back(cursor);
        int pos = insert_binary(cursor, searchkey, 0, cursor->size - 1, skiplow, equal);
        cursor = cursor->ptrs.at(pos);
    }
    return cursor;
}

int BPTreeWT::search_binary(NodeWT *cursor, string_view key, int low, int high, size_t &skiplow){
    size_t skiphigh = 0;
    size_t match;
    while (low <= high){
        int mid = low + (high - low) / 2;
        string pagekey = extract_key(cursor, mid, this->non_leaf_comp
        );
        int cmp;
        match = min(skiplow, skiphigh);
        cmp = lex_compare_skip(string(key), pagekey, &match);
        if (cmp > 0){
            skiplow = match;
        } else if (cmp < 0) {
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
                     int &totalBranching, unsigned long &totalKeyWTSize, int &totalPrefixSize){
    if (cursor != NULL){
        int currSize = 0;
        int prefixSize = 0;
        for (int i = 0; i < cursor->size; i++){
          currSize += cursor->keys.at(i).getSize();
          prefixSize += sizeof(cursor->keys.at(i).prefix);
        }
        totalKeyWTSize += currSize;
        numKeyWTs += cursor->size;
        totalPrefixSize += prefixSize;
        numNodeWTs += 1;
        if (cursor->IS_LEAF != true){
          totalBranching += cursor->ptrs.size();
          numNonLeaf += 1;
          for (int i = 0; i < cursor->size + 1; i++){
            getSize(cursor->ptrs[i], numNodeWTs, numNonLeaf, numKeyWTs, totalBranching, totalKeyWTSize, totalPrefixSize);
          }
        }
    }
}

int BPTreeWT::getHeight(NodeWT *cursor){
    if (cursor == NULL)
        return 0;
    if (cursor->IS_LEAF == true)
        return 1;
    int maxHeight = 0;
    for (int i = 0; i < cursor->size + 1; i++)
    {
        maxHeight = max(maxHeight, getHeight(cursor->ptrs.at(i)));
    }
    return maxHeight + 1;
}

void BPTreeWT::printTree(NodeWT *x, vector<bool> flag, bool compressed, int depth, bool isLast){
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
        printKeys_wt(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast){
        cout << "+--- ";
        printKeys_wt(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }else{
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