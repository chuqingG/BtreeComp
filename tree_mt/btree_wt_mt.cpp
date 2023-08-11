#include "btree_wt_mt.h"
#include "./compression/compression_wt.cpp"


// Initialise the BPTreeWTMT NodeWT
BPTreeWTMT::BPTreeWTMT(bool non_leaf_compression,
                bool suffix_compression){
    root = new NodeWT;
    non_leaf_comp = non_leaf_compression;
    suffix_comp = suffix_compression;
}

// Destructor of BPTreeWTMT
BPTreeWTMT::~BPTreeWTMT(){
    delete root;
}

// Function to get the root NodeWT
NodeWT *BPTreeWTMT::getRoot(){
    return root;
}


// Function to set the root of BPTreeDB2
void BPTreeWTMT::setRoot(NodeWT *newRoot)
{
    root = newRoot;
}

void BPTreeWTMT::lockRoot(accessMode mode)
{
    // If root changed in the middle, update to original root
    NodeWT *prevRoot = root;
    prevRoot->lock(mode);
    while (prevRoot != root)
    {
        prevRoot->unlock(mode);
        prevRoot = root;
        root->lock(mode);
    }
}

// Function to find any element
// in B+ Tree
int BPTreeWTMT::search(string_view key){
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, key, parents, skiplow);   
    int pos =  leaf != nullptr ?  search_binary(leaf, key, 0, leaf->size - 1, skiplow) : -1;
    leaf->unlock(READ);
    return pos;
}

// Function to peform range query on B+Tree
int BPTreeWTMT::searchRange(string min, string max){
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, min, parents, skiplow);
    NodeWT *prevleaf = nullptr;
    int pos =  leaf != nullptr ?  search_binary(leaf, min, 0, leaf->size - 1, skiplow) : -1;
    int entries = 0;
    string prevkey = min;
    // Keep searching till value > max or we reach end of tree
    while (leaf != nullptr) {
        if (pos == leaf->size) {
            pos = 0;
            prevleaf = leaf;
            leaf = leaf->next;
            prevleaf->unlock(READ);
            if (leaf == nullptr)
            {
                break;
            }
            prevkey = leaf->keys.at(0).value;
            leaf->lock(READ);
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
    if (leaf)
        leaf->unlock(READ);
    return entries;
}

// Function to peform backward scan on B+tree
// Backward scan first decompresses the node in the forward direction
// and then scans in the backward direction. This is done to avoid overhead
// of decompression in the backward direction.
int BPTreeWTMT::backwardScan(string min, string max) {
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, max, parents, skiplow);
    NodeWT *prevleaf = nullptr;
    int pos =  leaf != nullptr ?  search_binary(leaf, max, 0, leaf->size - 1, skiplow) : -1;
    int entries = 0;
    vector<string> decompressed_keys = decompress_keys(leaf, pos);
    // Keep searching till value > min or we reach end of tree
    while (leaf != nullptr)
    {
        if (decompressed_keys.size() == 0)
        {
            prevleaf = leaf;
            leaf = leaf->prev;
            prevleaf->unlock(READ);
            if (leaf == nullptr)
            {
                break;
            }
            leaf->lock(READ);
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
    if (leaf)
        leaf->unlock(READ);
    return entries;
}

//Decompress key in forward direction until a certain provided index
vector<string> BPTreeWTMT::decompress_keys(NodeWT *node, int pos) {
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

void BPTreeWTMT::insert(string x){
    lockRoot(WRITE);
    if (root->size == 0){
        int rid = rand();
        root->keys.push_back(KeyWT(x, 0, rid));
        root->IS_LEAF = true;
        root->size = 1;
        root->unlock(WRITE);
        return;
    }
    vector<NodeWT *> parents;
    size_t skiplow = 0;
    NodeWT *leaf = search_leaf_node(root, x, parents, skiplow);
    insert_leaf(leaf, parents, x);
}

void BPTreeWTMT::insert_nonleaf(NodeWT *node, vector<NodeWT *> &parents, int pos, splitReturnWT childsplit){
    node->lock(WRITE);
    // Move right if any splits occurred
    node = move_right(node, childsplit.promotekey);

    // Unlock right child
    childsplit.right->unlock(WRITE);
    // Unlock prev node
    childsplit.left->unlock(WRITE);
    
    if (check_split_condition(node)){
        splitReturnWT currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            NodeWT *newRoot = new NodeWT;
            int rid = rand();
            newRoot->keys.push_back(KeyWT(currsplit.promotekey, 0, rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            newRoot->level = node->level + 1;
            setRoot(newRoot);
            currsplit.right->unlock(WRITE);
            currsplit.left->unlock(WRITE);
        }else{
            NodeWT *parent = nullptr;
            if (pos >= 0) {
                parent = parents.at(pos);
            }
            else {
                //Fetch parents if new levels have been added
                bt_fetch_root(node, parents);
                if (parents.size() > 0)
                {
                    pos = parents.size() - 1;
                    parent = parents.at(pos);
                }
            }
            if (parent == nullptr) {
                currsplit.right->unlock(WRITE);
                currsplit.left->unlock(WRITE);
                return;
            }
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

        node->unlock(WRITE);
    }
}

void BPTreeWTMT::insert_leaf(NodeWT *leaf, vector<NodeWT *> &parents, string key){
    // Change shared lock to exclusive
    leaf->unlock(READ);
    leaf->lock(WRITE);
    // Move right if any splits occurred
    leaf = move_right(leaf, key);
    
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
            newRoot->level = leaf->level + 1;
            setRoot(newRoot);
            split.right->unlock(WRITE);
            leaf->unlock(WRITE);
        }else{
            //Fetch parents if new levels have been added
            if (parents.size() == 0)
            {
                bt_fetch_root(leaf, parents);
            }
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
        if(!equal) {
            leaf->size = leaf->size + 1;
            build_page_prefixes(leaf, insertpos, key); // Populate new prefixes
        }

        leaf->unlock(WRITE);
    }
}

int BPTreeWTMT::split_point(vector<KeyWT> allkeys){
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnWT BPTreeWTMT::split_nonleaf(NodeWT *node, vector<NodeWT *> parents, int pos, splitReturnWT childsplit){
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

    // Lock right node
    right->lock(WRITE);

    right->size = rightkeys.size();
    right->IS_LEAF = false;
    right->keys = rightkeys;
    right->ptrs = rightptrs;
    right->level = node->level;

    // Set high keys
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;
    
    // Set next pointers
    NodeWT *next = node->next;
    right->prev = node;
    if (next)
        next->lock(WRITE);
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;
    if (next)
        next->unlock(WRITE);

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

splitReturnWT BPTreeWTMT::split_leaf(NodeWT *node, vector<NodeWT *> &parents, string newkey){
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

    // Lock right node
    right->lock(WRITE);

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;
    right->level = node->level;

    // Set high keys
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;

    // Set next pointers
    NodeWT *next = node->next;
    right->prev = node;
    if (next)
        next->lock(WRITE);
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;
    if (next)
        next->unlock(WRITE);

    if (!equal && insertpos < split){
        build_page_prefixes(node, insertpos, newkey); // Populate new prefixes
    }
    build_page_prefixes(right, 0, firstright); // Populate new prefixes

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeWTMT::check_split_condition(NodeWT *node){
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

int BPTreeWTMT::insert_binary(NodeWT *cursor, string_view key, int low, int high, size_t &skiplow, bool &equal){
    stringstream ss;
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

NodeWT* BPTreeWTMT::search_leaf_node(NodeWT *root, string_view key, vector<NodeWT *> &parents, size_t &skiplow)
{
    // Tree is empty
    if (root->size == 0){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeWT *cursor = root;
    cursor->unlock(WRITE);
    cursor->lock(READ);

    // Till we reach leaf node
    while (!cursor->IS_LEAF){
        string_view searchkey = key;
         // Acquire read lock before scanning node
        NodeWT *nextPtr = scan_node(cursor, string(searchkey), skiplow);
        if (nextPtr != cursor->next)
        {
            parents.push_back(cursor);
        }
         // Release read lock after scanning node
        cursor->unlock(READ);
        // Acquire lock on next ptr
        nextPtr->lock(READ);
        cursor = nextPtr;
    }
    return cursor;
}

int BPTreeWTMT::search_binary(NodeWT *cursor, string_view key, int low, int high, size_t &skiplow){
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

NodeWT* BPTreeWTMT::scan_node(NodeWT *node, string key, size_t &skiplow){
    if (node->highkey.compare(MAXHIGHKEY) != 0 && node->highkey.compare(key) <= 0)
    {
        skiplow = 0;
        return node->next;
    }
    else if (node->IS_LEAF)
        return node;
    else
    {
        bool equal = false;
        int pos = insert_binary(node, key, 0, node->size - 1, skiplow, equal);
        return node->ptrs.at(pos);
    }
}

NodeWT* BPTreeWTMT::move_right(NodeWT *node, string key)
{
    NodeWT *current = node;
    NodeWT *nextPtr;
    size_t skiplow = 0;    //Skip low is not relevant here
    while ((nextPtr = scan_node(current, string(key), skiplow)) == current->next)
    {
        current->unlock(WRITE);
        nextPtr->lock(WRITE);
        current = nextPtr;
    }

    return current;
}

//Fetch root if it was changed by concurrent threads
void BPTreeWTMT::bt_fetch_root(NodeWT *currRoot, vector<NodeWT *> &parents){
    NodeWT *cursor = root;

    cursor->lock(READ);
    if (cursor->level == currRoot->level)
    {
        cursor->unlock(READ);
        return;
    }

    string key = currRoot->keys.at(0).value;
    NodeWT *nextPtr;
    size_t skiplow = 0; //skip low is not relevant here

    while (cursor->level != currRoot->level)
    {
        NodeWT *nextPtr = scan_node(cursor, key, skiplow);
        if (nextPtr != cursor->next)
        {
            parents.push_back(cursor);
        }

        cursor->unlock(READ);

        if (nextPtr->level != currRoot->level)
        {
            nextPtr->lock(READ);
        }

        cursor = nextPtr;
    }

    return;
}

/*
================================================
=============statistic function & printer=======
================================================
*/

void BPTreeWTMT::getSize(NodeWT *cursor, int &numNodeWTs, int &numNonLeaf, int &numKeyWTs,
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


int BPTreeWTMT::getHeight(NodeWT *cursor){
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

void BPTreeWTMT::printTree(NodeWT *x, vector<bool> flag, bool compressed, int depth, bool isLast){
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