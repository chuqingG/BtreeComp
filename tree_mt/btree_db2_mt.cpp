#include "btree_db2_mt.h"

PrefixMetaData::PrefixMetaData(){
    prefix = "";
    low = 0;
    high = 0;
}

PrefixMetaData::PrefixMetaData(string p, int l, int h){
    prefix = p;
    low = l;
    high = h;
}

// Initialise the BPTreeDB2MT DB2Node
BPTreeDB2MT::BPTreeDB2MT(){
    root = new DB2Node;
}

// Destructor of BPTreeDB2MT tree
BPTreeDB2MT::~BPTreeDB2MT(){
    delete root;
}

// Function to get the root DB2Node
DB2Node *BPTreeDB2MT::getRoot(){
    return root;
}

// Function to set the root of BPTreeDB2
void BPTreeDB2MT::setRoot(DB2Node *newRoot)
{
    root = newRoot;
}

void BPTreeDB2MT::lockRoot(accessMode mode)
{
    // If root changed in the middle, update to original root
    DB2Node *prevRoot = root;
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
int BPTreeDB2MT::search(string_view key, bool print){
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(root, key, parents, print);
    int pos;
    if (leaf == nullptr){
        pos = -1;
    }
    else {
        int metadatapos = search_prefix_metadata(leaf, key);
        if (metadatapos == -1){
            pos =  -1;
        }
        else {
            PrefixMetaData currentMetadata = leaf->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            string_view compressed_key = key.substr(prefix.length());
            pos = search_binary(leaf, compressed_key, currentMetadata.low, currentMetadata.high);
        }
    }
    
    // Release read lock after scanning node
    leaf->unlock(READ);
    return pos;
}

// Function to peform range query on B+Tree
int BPTreeDB2MT::searchRange(string min, string max){
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(root, min, parents);
    DB2Node *prevleaf = nullptr;
    int pos, metadatapos;
    int entries = 0;
    if (leaf == nullptr){
        pos = -1;
    }
    else {
        metadatapos = search_prefix_metadata(leaf, min);
        if (metadatapos == -1){
            pos =  -1;
        }
        else {
            PrefixMetaData currentMetadata = leaf->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            string compressed_key = min.substr(prefix.length());
            pos = search_binary(leaf, compressed_key, currentMetadata.low, currentMetadata.high);
        }
    }
    // Keep searching till value > max or we reach end of tree
    while (pos != -1 && leaf != nullptr) {
        if (pos == leaf->size) {
            pos = 0;
            metadatapos = 0;
            prevleaf = leaf;
            leaf = leaf->next;
            prevleaf->unlock(READ);
            if (leaf == nullptr)
            {
                break;
            }
            leaf->lock(READ);
        }
        
        if(pos > leaf->prefixMetadata.at(metadatapos).high)
        {
            metadatapos += 1;
        }
        string leafkey = leaf->prefixMetadata.at(metadatapos).prefix + leaf->keys.at(pos).value;
        
        if (lex_compare(leafkey, max) > 0)
        {
            break;
        }
        entries+=leaf->keys.at(pos).ridList.size();
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
int BPTreeDB2MT::backwardScan(string min, string max) {
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(root, max, parents);
    DB2Node *prevleaf = nullptr;
    int pos, metadatapos;
    int entries = 0;
    if (leaf == nullptr){
        pos = -1;
    }
    else {
        metadatapos = search_prefix_metadata(leaf, max);
        if (metadatapos == -1){
            pos =  -1;
        }
        else {
            PrefixMetaData currentMetadata = leaf->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            string compressed_key = max.substr(prefix.length());
            pos = search_binary(leaf, compressed_key, currentMetadata.low, currentMetadata.high);
        }
    }
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
vector<string> BPTreeDB2MT::decompress_keys(DB2Node *node, int pos) {
    vector<string> decompressed_keys;
    if (node == nullptr)
        return decompressed_keys;
    int size = pos == -1 ? node->keys.size() - 1 : pos; 
    int ind;
    for(int i = 0; i < node->prefixMetadata.size(); i++)
    {
        for(ind = node->prefixMetadata.at(i).low ; ind <= min(node->prefixMetadata.at(i).high, size); ind++)
        {
            string key = node->prefixMetadata.at(i).prefix + node->keys.at(ind).value;
            decompressed_keys.push_back(key);
        }
        if(ind > size)
            break;
    }
    return decompressed_keys;
}

void BPTreeDB2MT::insert(string x){
    lockRoot(WRITE);
    if (root->size == 0){
        int rid = rand();
        root->keys.push_back(Key(x, rid));
        root->IS_LEAF = true;
        root->size = 1;
        PrefixMetaData metadata = PrefixMetaData("", 0, 0);
        root->prefixMetadata.push_back(metadata);
        root->unlock(WRITE);
        return;
    }
    vector<DB2Node *> parents;
    DB2Node *leaf = search_leaf_node(root, x, parents);
    insert_leaf(leaf, parents, x);
}

void BPTreeDB2MT::insert_nonleaf(DB2Node *node, vector<DB2Node *> &parents, int pos, splitReturnDB2 childsplit){
    node->lock(WRITE);
    // Move right if any splits occurred
    node = move_right(node, childsplit.promotekey);

     // Unlock right child
    childsplit.right->unlock(WRITE);
    // Unlock prev node
    childsplit.left->unlock(WRITE);

    if (check_split_condition(node)){
        apply_prefix_optimization(node);
    }
    if (check_split_condition(node)){
        splitReturnDB2 currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            DB2Node *newRoot = new DB2Node;
            int rid = rand();
            newRoot->keys.push_back(Key(currsplit.promotekey,rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            PrefixMetaData metadata = PrefixMetaData("", 0, 0);
            newRoot->prefixMetadata.push_back(metadata);
            newRoot->level = node->level + 1;
            setRoot(newRoot);
            currsplit.right->unlock(WRITE);
            currsplit.left->unlock(WRITE);
        }else{
            DB2Node *parent = nullptr;
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
        int rid = rand();
        bool equal = false;
        int insertpos = find_insert_pos(node, promotekey, this->insert_binary, equal);
        node->keys.insert(node->keys.begin() + insertpos, Key(promotekey, rid));
        node->ptrs.insert(node->ptrs.begin() + insertpos + 1, childsplit.right);
        node->size = node->size + 1;
        node->unlock(WRITE);
    }
}

void BPTreeDB2MT::insert_leaf(DB2Node *leaf, vector<DB2Node *> &parents, string key){
     // Change shared lock to exclusive
    leaf->unlock(READ);
    leaf->lock(WRITE);
    // Move right if any splits occurred
    leaf = move_right(leaf, key);
    
    if (check_split_condition(leaf)){
        apply_prefix_optimization(leaf);
    }
    if (check_split_condition(leaf)){
        splitReturnDB2 split = split_leaf(leaf, parents, key);
        if (leaf == root){
            DB2Node *newRoot = new DB2Node;
            int rid = rand();
            newRoot->keys.push_back(Key(split.promotekey, rid));
            newRoot->ptrs.push_back(split.left);
            newRoot->ptrs.push_back(split.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            PrefixMetaData metadata = PrefixMetaData("", 0, 0);
            newRoot->prefixMetadata.push_back(metadata);
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
            DB2Node *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }else{
        int insertpos;
        int rid = rand();
        bool equal = false;
        insertpos = find_insert_pos(leaf, key, this->insert_binary, equal);
        if(equal){
            leaf->keys.at(insertpos - 1).addRecord(rid);
        }
        else {
            leaf->keys.insert(leaf->keys.begin() + insertpos, Key(key, rid));
        }
        if(!equal){
            leaf->size = leaf->size + 1;
        }
        leaf->unlock(WRITE);
    }
}

int BPTreeDB2MT::split_point(vector<Key> allkeys){
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnDB2 BPTreeDB2MT::split_nonleaf(DB2Node *node, vector<DB2Node *> parents, int pos, splitReturnDB2 childsplit){
    splitReturnDB2 newsplit;
    DB2Node *right = new DB2Node;
    string promotekey = childsplit.promotekey;
    int insertpos;
    int rid = rand();
    bool equal = false;
    insertpos = find_insert_pos(node, promotekey, this->insert_binary, equal);
    vector<Key> allkeys;
    vector<DB2Node *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }
    //Non-leaf nodes won't have duplicates
    allkeys.push_back(Key(promotekey, rid));
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

    vector<Key> leftkeys;
    vector<Key> rightkeys;
    vector<DB2Node *> leftptrs;
    vector<DB2Node *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    // Lock right node
    right->lock(WRITE);

    db2split splitresult = perform_split(node, split, false);
    node->prefixMetadata = splitresult.leftmetadatas;
    right->prefixMetadata = splitresult.rightmetadatas;
    newsplit.promotekey = splitresult.splitprefix + allkeys.at(split).value;

    node->size = leftkeys.size();
    node->keys = leftkeys;
    node->ptrs = leftptrs;


    right->size = rightkeys.size();
    right->IS_LEAF = false;
    right->keys = rightkeys;
    right->ptrs = rightptrs;
    right->level = node->level;

     // Set high keys
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;

    // Set next pointers
    DB2Node *next = node->next;
    right->prev = node;
    if (next)
        next->lock(WRITE);
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;
    if (next)
        next->unlock(WRITE);

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnDB2 BPTreeDB2MT::split_leaf(DB2Node *node, vector<DB2Node *> &parents, string newkey){
    splitReturnDB2 newsplit;
    DB2Node *right = new DB2Node;
    int insertpos;
    int rid = rand();
    bool equal = false;
    insertpos = find_insert_pos(node, newkey, this->insert_binary, equal);

    vector<Key> allkeys;

    if(equal){
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        allkeys.push_back(Key(newkey, rid));
        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(node->keys.at(i));
        }
    }

    int split = split_point(allkeys);

    vector<Key> leftkeys;
    vector<Key> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));

     // Lock right node
    right->lock(WRITE);

    db2split splitresult = perform_split(node, split, true);
    node->prefixMetadata = splitresult.leftmetadatas;
    right->prefixMetadata = splitresult.rightmetadatas;

    string firstright = rightkeys.at(0).value;
    PrefixMetaData firstrightmetadata = right->prefixMetadata.at(0);
    firstright = firstrightmetadata.prefix + firstright;
    newsplit.promotekey = firstright;

    node->size = leftkeys.size();
    node->keys = leftkeys;

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;
    right->level = node->level;

     // Set high keys
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;

    // Set next pointers
    DB2Node *next = node->next;
    right->prev = node;
    if (next)
        next->lock(WRITE);
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;
    if (next)
        next->unlock(WRITE);

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeDB2MT::check_split_condition(DB2Node *node){
    int currspace = 0;
#ifdef SPLIT_STRATEGY_SPACE
    for (uint32_t i = 0; i < node->prefixMetadata.size(); i++){
        currspace += node->prefixMetadata.at(i).prefix.length();
    }
    for (uint32_t i = 0; i < node->keys.size(); i++){
        currspace += node->keys.at(i).getSize();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeDB2MT::insert_binary(DB2Node *cursor, string_view key, int low, int high, bool &equal){
    while (low <= high){
        int mid = low + (high - low) / 2;
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

DB2Node* BPTreeDB2MT::search_leaf_node(DB2Node *root, string_view key, vector<DB2Node *> &parents, bool print)
{
    if(print)
        cout << "Search for " << key << endl;
    // Tree is empty
    if (root->size == 0){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    DB2Node *cursor = root;
    cursor->unlock(WRITE);
    cursor->lock(READ);

    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        string_view searchkey = key;
        if(print)
            cout << "Cursor first key " << cursor->prefixMetadata.at(0).prefix + cursor->keys.at(0).value << endl;

         // Acquire read lock before scanning node
        DB2Node *nextPtr = scan_node(cursor, string(searchkey),print);
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
    if(print)
        cout << "Leaf first key " << cursor->prefixMetadata.at(0).prefix + cursor->keys.at(0).value << endl;
    return cursor;
}

int BPTreeDB2MT::search_binary(DB2Node *cursor, string_view key, int low, int high){
    while (low <= high){
        int mid = low + (high - low) / 2;
        int cmp = lex_compare(string(key), cursor->keys.at(mid).value);
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    cout << "key:" << key << endl;
    cout << "node: " << cursor->keys.at(0).value << endl;
    return -1;
}

DB2Node* BPTreeDB2MT::scan_node(DB2Node *node, string key, bool print){
    if (node->highkey.compare(MAXHIGHKEY) != 0 && node->highkey.compare(key) <= 0)
    {
        return node->next;
    }
    else if (node->IS_LEAF)
        return node;
    else
    {
        string searchkey = key;
        int pos = -1;
        int metadatapos = insert_prefix_metadata(node, searchkey);
        if (metadatapos == node->prefixMetadata.size()){
            pos = node->size;
        } else {
            bool equal = false;
            PrefixMetaData currentMetadata = node->prefixMetadata.at(metadatapos);
            string prefix = currentMetadata.prefix;
            int prefixcmp = key.compare(0, prefix.length(), prefix);
            if (prefixcmp != 0){
                pos = prefixcmp > 0 ? currentMetadata.high + 1 : currentMetadata.low;
            }else{
                searchkey = key.substr(prefix.length());
                pos = insert_binary(node, searchkey, currentMetadata.low, currentMetadata.high, equal);
            }
            if(print)
                cout << "Position chosen " << pos << endl;
        }
        return node->ptrs.at(pos);
    }
}

DB2Node* BPTreeDB2MT::move_right(DB2Node *node, string key)
{
    DB2Node *current = node;
    DB2Node *nextPtr;
    while ((nextPtr = scan_node(current, string(key))) == current->next)
    {
        current->unlock(WRITE);
        nextPtr->lock(WRITE);
        current = nextPtr;
    }

    return current;
}

//Fetch root if it was changed by concurrent threads
void BPTreeDB2MT::bt_fetch_root(DB2Node *currRoot, vector<DB2Node *> &parents){
    DB2Node *cursor = root;

    cursor->lock(READ);
    if (cursor->level == currRoot->level)
    {
        cursor->unlock(READ);
        return;
    }

    string key = currRoot->prefixMetadata.at(0).prefix + currRoot->keys.at(0).value;
    DB2Node *nextPtr;

    while (cursor->level != currRoot->level)
    {
        DB2Node *nextPtr = scan_node(cursor, key);
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

void BPTreeDB2MT::getSize(DB2Node *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                     int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize){
    if (cursor != NULL){
        int currSize = 0;
        int prefixSize = 0;
        for (uint32_t i = 0; i < cursor->prefixMetadata.size(); i++){
            currSize += cursor->prefixMetadata.at(i).prefix.length();
            prefixSize += cursor->prefixMetadata.at(i).prefix.length();
        }
        for (int i = 0; i < cursor->size; i++){
            currSize += cursor->keys.at(i).getSize();
        }
        totalKeySize += currSize;
        numKeys += cursor->size;
        totalPrefixSize += prefixSize;
        numNodes += 1;
        if (cursor->IS_LEAF != true){
            totalBranching += cursor->ptrs.size();
            numNonLeaf += 1;
            for (int i = 0; i < cursor->size + 1; i++){
                getSize(cursor->ptrs[i], numNodes, numNonLeaf, numKeys, totalBranching, totalKeySize, totalPrefixSize);
            }
        }
    }
}

int BPTreeDB2MT::getHeight(DB2Node *cursor){
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

void BPTreeDB2MT::printTree(DB2Node *x, vector<bool> flag, bool compressed, int depth, bool isLast)
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
        printKeys_db2(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast){
        cout << "+--- ";
        printKeys_db2(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }else{
        cout << "+--- ";
        printKeys_db2(x, compressed);
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