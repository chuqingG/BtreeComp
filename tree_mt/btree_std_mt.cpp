#include "btree_std_mt.h"
#include "./compression/compression_std.cpp"


// Initialise the BPTreeMT Node
BPTreeMT::BPTreeMT(bool head_compression, 
                bool tail_compression){
    root = new Node;
    head_comp = head_compression;
    tail_comp = tail_compression;
}

// Destructor of BPTreeMT
BPTreeMT::~BPTreeMT(){
    delete root;
}

// Function to get the root Node
Node *BPTreeMT::getRoot(){
    return root;
}

// Function to set the root of BPTree
void BPTreeMT::setRoot(Node *newRoot){
    root = newRoot;
}

void BPTreeMT::lockRoot(accessMode mode){
    // If root changed in the middle, update to original root
    Node *prevRoot = root;
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
int BPTreeMT::search(const char* key, bool print){
    vector<Node *> parents;

    shared_ptr<char[]> temp_ptr(new char[strlen(key) + 1]);
    // memcpy(temp_ptr.get(), key, strlen(key) + 1);
    strcpy(temp_ptr.get(), key);

    Node *leaf = search_leaf_node(root, temp_ptr, parents,print);
    char* searchkey;
    int searchpos;
    if (this->head_comp){
        searchpos = search_binary(leaf, temp_ptr.get() + leaf->prefix.length(), 0, leaf->size - 1);
    }else{
        searchpos = search_binary(leaf, temp_ptr.get(), 0, leaf->size - 1);
    }
    int pos = leaf != nullptr ? searchpos : -1;
    // Release read lock after scanning node
    leaf->unlock(READ);
    return pos;
}

void BPTreeMT::insert(char* x){
    //Lock root before writing
    lockRoot(WRITE);
    // if(char_cmp(x, "OwwumDpcML9x1p5dPCg7uaMq7SVyxz0EjOR2QrNA5hsk9Bn2TrrMra03vmTuJrkw") == 0){
    //     cout << "1" << endl;
    // }else if(char_cmp(x, "OuX1szYAoxuICsffCoRCoWZdTUpZrvVTcHQVtM43hoC0RMTXjbD0VZ7xj9wske9N") == 0){
    //     cout << "2" << endl;
    // }else if(char_cmp(x, "OuUHatD7qWNc07f3sNHdJ07YgfMpwVLydmAyEeFtQ4PC3I2ENGi4rBBlJSO5VHeu") == 0){
    //     cout << "3" << endl;
    // }
    if (root->size == 0){
        int rid = rand();
        root->keys.push_back(Key_c(x, rid));
        root->IS_LEAF = true;
        root->size = 1;
        root->unlock(WRITE);
        return;
    }
    vector<Node *> parents;
    shared_ptr<char[]> temp_ptr(new char[strlen(x) + 2]);
    // memcpy(temp_ptr.get(), x, strlen(x) + 1);
    strcpy(temp_ptr.get(), x);

    Node *leaf = search_leaf_node(root, temp_ptr, parents);
    insert_leaf(leaf, parents, temp_ptr);
}

#ifdef TIMEDEBUG
void BPTreeMT::insert_time(char* x, double* calc, double* update){
    //Lock root before writing
    lockRoot(WRITE);

    if (root->size == 0){
        int rid = rand();
        root->keys.push_back(Key_c(x, rid));
        root->IS_LEAF = true;
        root->size = 1;
        root->unlock(WRITE);
        return;
    }
    vector<Node *> parents;
    shared_ptr<char[]> temp_ptr(new char[strlen(x) + 1]);
    memcpy(temp_ptr.get(), x, strlen(x) + 1);

    Node *leaf = search_leaf_node(root, temp_ptr, parents);
    insert_leaf(leaf, parents, temp_ptr);
}
#endif

void BPTreeMT::insert_nonleaf(Node *node, vector<Node *> &parents, int pos, splitReturn childsplit){
    node->lock(WRITE);
    // Move right if any splits occurred
    shared_ptr<char[]> pkey(new char[childsplit.promotekey.size() + 2]);
    // memcpy(pkey.get(), childsplit.promotekey.data(), childsplit.promotekey.size() + 1);
    strcpy(pkey.get(), childsplit.promotekey.data());

    node = move_right(node, pkey);

     // Unlock right child
    childsplit.right->unlock(WRITE);
    // Unlock prev node
    childsplit.left->unlock(WRITE);
    
    if (check_split_condition(node)){
        splitReturn currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            Node *newRoot = new Node;
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(currsplit.promotekey), rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            newRoot->level = node->level + 1;
            setRoot(newRoot);
            currsplit.right->unlock(WRITE);
            currsplit.left->unlock(WRITE);
        }else{
            Node *parent = nullptr;
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
        int insertpos;
        bool equal = false;
        int rid = rand();
        if (this->head_comp) {

            // promotekey = promotekey.substr(node->prefix.length());
            insertpos = insert_binary(node, promotekey.data() + node->prefix.length(), 0, node->size - 1, equal);
        } else {
            insertpos = insert_binary(node, promotekey.data(), 0, node->size - 1, equal);
        }
        vector<Key_c> allkeys;

        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }

        // insert new here, may need to create new to deallocate
        char* newkey_ptr;
        // memcpy(newkey_ptr, promotekey.data(), promotekey.size() + 1);
        // strcpy(newkey_ptr, promotekey.data());
        if(head_comp){
            newkey_ptr = new char[promotekey.size() - node->prefix.length() + 1];
            strcpy(newkey_ptr, promotekey.data() + node->prefix.length());
        }else{
            newkey_ptr = new char[promotekey.size() + 1];
            strcpy(newkey_ptr, promotekey.data());
        }

        // if(strlen(newkey_ptr) > 64)
        //     cout << "invalid key 1" << endl;
        allkeys.push_back(Key_c(newkey_ptr, rid));
        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(node->keys.at(i));
        }

        vector<Node *> allptrs;
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
        node->unlock(WRITE);
    }
}

void BPTreeMT::insert_leaf(Node *leaf, vector<Node *> &parents, shared_ptr<char[]> key){
    // Change shared lock to exclusive
    leaf->unlock(READ);
    leaf->lock(WRITE);
    // Move right if any splits occurred
    leaf = move_right(leaf, key);
    
    if (check_split_condition(leaf)){
        splitReturn split = split_leaf(leaf, parents, key);
        if (leaf == root){
            Node *newRoot = new Node;
            int rid = rand();
            newRoot->keys.push_back(Key_c(string_to_char(split.promotekey), rid));
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
            Node *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }else{
        int insertpos;
        int rid = rand();
        bool equal = false;
        if (this->head_comp) {
            // key = key.substr(leaf->prefix.length());
            insertpos = insert_binary(leaf, key.get() + leaf->prefix.length(), 0, leaf->size - 1, equal);
        } else {
            insertpos = insert_binary(leaf, key.get(), 0, leaf->size - 1, equal);
        }
        vector<Key_c> allkeys;
        if (equal) {
            allkeys = leaf->keys;
            allkeys.at(insertpos - 1).addRecord(rid);
        }
        else {
            for (int i = 0; i < insertpos; i++){
                allkeys.push_back(leaf->keys.at(i));
            }
            // insert new here, may need to create new to deallocate
            char* key_ptr = new char[strlen(key.get()) + 1];
            // memcpy(key_ptr, key.get(), strlen(key.get()) + 1);
            // strcpy(key_ptr, key.get());
            if(this->head_comp){
                strcpy(key_ptr, key.get() + leaf->prefix.length());
            }else{
                strcpy(key_ptr, key.get());
            }

            // if(strlen(key_ptr) > 64)
            //     cout << "invalid key 2" << endl;

            allkeys.push_back(Key_c(key_ptr, rid));

            for (int i = insertpos; i < leaf->size; i++){
                allkeys.push_back(leaf->keys.at(i));
            }
        }
        leaf->keys = allkeys;
        if (!equal) {
            leaf->size = leaf->size + 1;
        }
        leaf->unlock(WRITE);
    }
}

int BPTreeMT::split_point(vector<Key_c> allkeys)
{
    int size = allkeys.size();
    int bestsplit = size / 2;
    if (this->tail_comp){
        int split_range_low = size / 3;
        int split_range_high = 2 * size / 3;
        // Representing 16 bytes of the integer
        int minlen = INT16_MAX;
        for (int i = split_range_low; i < split_range_high - 1; i++)
        {
            if (i + 1 < size)
            {
                string compressed = tail_compress(allkeys.at(i).value,
                                                  allkeys.at(i + 1).value);
                if (compressed.length() < minlen)
                {
                    minlen = compressed.length();
                    bestsplit = i + 1;
                }
            }
        }
    }
    return bestsplit;
}

splitReturn BPTreeMT::split_nonleaf(Node *node, vector<Node *> parents, int pos, splitReturn childsplit){
    splitReturn newsplit;
    Node *right = new Node;
    string promotekey = childsplit.promotekey;
    string prevprefix = node->prefix;
    int insertpos;
    int rid = rand();
    bool equal = false;
    string splitprefix;
    if (this->head_comp) {
        promotekey = promotekey.substr(node->prefix.length());
        insertpos = insert_binary(node, promotekey.data(), 0, node->size - 1, equal);
    } else {
        insertpos = insert_binary(node, promotekey.data(), 0, node->size - 1, equal);
    }
    vector<Key_c> allkeys;
    vector<Node *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }

    //Non-leaf nodes won't have duplicates

    // insert new here, may need to create new to deallocate
    char* newkey_ptr = new char[promotekey.size() + 2];
    strcpy(newkey_ptr, promotekey.data());
    allkeys.push_back(Key_c(newkey_ptr, rid));

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

    vector<Key_c> leftkeys;
    vector<Key_c> rightkeys;
    vector<Node *> leftptrs;
    vector<Node *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    // Lock right node
    right->lock(WRITE);

    if(this->head_comp){
        newsplit.promotekey = node->prefix + allkeys.at(split).value;
        // string lower_bound = node->lowkey;
        // string upper_bound = node->highkey;
        string leftprefix = head_compression_find_prefix(node->lowkey, newsplit.promotekey.data());
        if(leftprefix.length() > prevprefix.length())
            prefix_compress_vector(leftkeys, prevprefix, leftprefix.length());
        node->prefix = leftprefix;
        if(char_cmp(node->highkey, MAXHIGHKEY) != 0){
            string rightprefix = head_compression_find_prefix(newsplit.promotekey.data(), node->highkey);
            if(rightprefix.length() > prevprefix.length())
                prefix_compress_vector(rightkeys, prevprefix, rightprefix.length());
            right->prefix = rightprefix;
        }
        else{
            right->prefix = prevprefix;
        }
    } else {
        newsplit.promotekey = allkeys.at(split).value;
    }

    node->size = leftkeys.size();
    node->keys = leftkeys;
    node->ptrs = leftptrs;

    right->size = rightkeys.size();
    right->IS_LEAF = false;
    right->keys = rightkeys;
    right->ptrs = rightptrs;
    right->level = node->level;

    // Set high keys
    //TODO: may change high/lowkey to shared_ptr
    right->highkey = node->highkey;
    node->highkey = string_to_char(newsplit.promotekey);
    right->lowkey = string_to_char(newsplit.promotekey);

    // Set next pointers
    Node *next = node->next;
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

splitReturn BPTreeMT::split_leaf(Node *node, vector<Node *> &parents, shared_ptr<char[]> newkey){
    splitReturn newsplit;
    Node *right = new Node;
    string prevprefix = node->prefix;
    int insertpos;
    int rid = rand();
    bool equal = false;
    if (this->head_comp) {
        // newkey = newkey.substr(node->prefix.length());
        insertpos = insert_binary(node, newkey.get() + node->prefix.length(), 0, node->size - 1, equal);
    } else {
        insertpos = insert_binary(node, newkey.get(), 0, node->size - 1, equal);
    }

    vector<Key_c> allkeys;

    if(equal) {
        allkeys = node->keys;
        allkeys.at(insertpos - 1).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        // insert new here, may need to create new to deallocate
        char* newkey_ptr = new char[strlen(newkey.get()) + 2];
        // memcpy(newkey_ptr, newkey.get(), strlen(newkey.get()) + 1);
        // strcpy(newkey_ptr, newkey.get());
        if(this->head_comp){
            strcpy(newkey_ptr, newkey.get() + node->prefix.length());
        }else{
            strcpy(newkey_ptr, newkey.get());
        }
        // if(strlen(newkey_ptr) > 64)
        //     cout << "invalid key 4" << endl;

        allkeys.push_back(Key_c(newkey_ptr, rid));
        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(node->keys.at(i));
        }
    }

    int split = split_point(allkeys);

    vector<Key_c> leftkeys;
    vector<Key_c> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));
    
    string firstright("");

    if (this->tail_comp) {
        string lastleft(leftkeys.at(leftkeys.size() - 1).value);
        firstright += rightkeys.at(0).value;
        if (this->head_comp) {
            lastleft = node->prefix + lastleft;
            firstright = node->prefix + firstright;
        } 
        newsplit.promotekey = tail_compress(lastleft.data(), firstright.data());
    } else {
        firstright += rightkeys.at(0).value;
        if (this->head_comp) {
            firstright = node->prefix + firstright;
        } 
        newsplit.promotekey = firstright;
    }

    // if(newsplit.promotekey.size() > 64)
    //     cout << "invalid promotekey" << endl;

    // Lock right node
    right->lock(WRITE);

    if (this->head_comp) {
        string lower_bound = node->lowkey;
        string upper_bound = node->highkey;
        string leftprefix = head_compression_find_prefix(
                                node->lowkey, newsplit.promotekey.data());
        if(leftprefix.length() > prevprefix.length()){
            // if(memcmp(leftprefix.data(), prevprefix.data(), prevprefix.length()*sizeof(char)) != 0)
            //     cout << "error prefix" << endl;
            prefix_compress_vector(leftkeys, prevprefix, leftprefix.length());
        }
        node->prefix = leftprefix;
    
        if(strcmp(node->highkey, MAXHIGHKEY) != 0){
            string rightprefix = head_compression_find_prefix(
                                    newsplit.promotekey.data(), node->highkey);
            if(rightprefix.length() > prevprefix.length())
                prefix_compress_vector(rightkeys, prevprefix, rightprefix.length());
            right->prefix = rightprefix;
        }
        else{
            right->prefix = prevprefix;
        }
    }

    node->size = leftkeys.size();
    node->keys = leftkeys;

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;
    right->level = node->level;

    // Set high keys
    //TODO: may change high/lowkey to shared_ptr
    right->highkey = node->highkey;
    node->highkey = string_to_char(newsplit.promotekey);
    right->lowkey = string_to_char(newsplit.promotekey);

    // Set next pointers
    Node *next = node->next;
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

bool BPTreeMT::check_split_condition(Node *node){
#ifdef SPLIT_STRATEGY_SPACE
    int currspace = node->prefix.length();
    for (uint32_t i = 0; i < node->keys.size(); i++)
    {
        currspace += node->keys.at(i).getSize();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeMT::insert_binary(Node *cursor, const char* key, int low, int high, bool &equal){
    while (low <= high){
        int mid = low + (high - low) / 2;
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
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

Node* BPTreeMT::search_leaf_node(Node *root, shared_ptr<char[]> key, vector<Node *> &parents,bool print)
{
    if(print)
        cout << "Search for " << key.get() << endl;
    // Tree is empty
    if (root->size == 0){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    Node *cursor = root;
    cursor->unlock(WRITE);
    cursor->lock(READ);

    // Till we reach leaf node
    while (!cursor->IS_LEAF){
        // string_view searchkey = key;
        if(print)
            cout << "Cursor first key " << cursor->prefix << cursor->keys.at(0).value << endl;
         // Acquire read lock before scanning node
        Node *nextPtr = scan_node(cursor, key, print);
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
        cout << "Leaf first key " << cursor->prefix << cursor->keys.at(0).value << endl;
    return cursor;
}

int BPTreeMT::search_binary(Node *cursor, const char* key, int low, int high){
    while (low <= high){
        int mid = low + (high - low) / 2;
        int cmp = char_cmp(key, cursor->keys.at(mid).value);
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}


Node* BPTreeMT::scan_node(Node *node, shared_ptr<char[]> key, bool print){
    if (char_cmp(node->highkey, MAXHIGHKEY) != 0 && char_cmp(node->highkey, key.get()) <= 0){
        return node->next;
    }
    else if (node->IS_LEAF)
        return node;
    else{
        // string_view searchkey = key;
        // char* searchkey;
        int pos;
        bool equal = false;
        if(this->head_comp){
            pos = insert_binary(node, key.get() + node->prefix.length(), 
                                0, node->size - 1, equal);
        }else{
            pos = insert_binary(node, key.get(), 
                                0, node->size - 1, equal);
        }
        if(print)
            cout << "Position chosen pos  " << pos << endl;
        return node->ptrs.at(pos);
    }
}

Node* BPTreeMT::move_right(Node *node, shared_ptr<char[]> key)
{
    Node *current = node;
    Node *nextPtr;
    while ((nextPtr = scan_node(current, key)) == current->next)
    {
        current->unlock(WRITE);
        nextPtr->lock(WRITE);
        current = nextPtr;
    }

    return current;
}

//Fetch root if it was changed by concurrent threads
void BPTreeMT::bt_fetch_root(Node *currRoot, vector<Node *> &parents){
    Node *cursor = root;

    cursor->lock(READ);
    if (cursor->level == currRoot->level)
    {
        cursor->unlock(READ);
        return;
    }

    string key_str = currRoot->prefix + currRoot->keys.at(0).value;
    shared_ptr<char[]> key(new char[key_str.size() + 1]);
    // memcpy(key.get(), key_str.data(), key_str.size() + 1);
    strcpy(key.get(), key_str.data());

    Node *nextPtr;

    while (cursor->level != currRoot->level)
    {
        Node *nextPtr = scan_node(cursor, key);
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

void BPTreeMT::getSize(Node *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                     int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize){
    if (cursor != NULL){
        int currSize = cursor->prefix.length();
        int prefixSize = cursor->prefix.length();
        for (int i = 0; i < cursor->size; i++)
        {
            currSize += cursor->keys.at(i).getSize();
        }
        totalKeySize += currSize;
        numKeys += cursor->size;
        totalPrefixSize += prefixSize;
        numNodes += 1;
        if (cursor->IS_LEAF != true)
        {
            totalBranching += cursor->ptrs.size();
            numNonLeaf += 1;
            for (int i = 0; i < cursor->size + 1; i++)
            {
                getSize(cursor->ptrs[i], numNodes, numNonLeaf, numKeys, totalBranching, totalKeySize, totalPrefixSize);
            }
        }
    }
}

int BPTreeMT::getHeight(Node *cursor){
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

void BPTreeMT::printTree(Node *x, vector<bool> flag, bool compressed, int depth, bool isLast)
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
        printKeys(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast){
        cout << "+--- ";
        printKeys(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }else{
        cout << "+--- ";
        printKeys(x, compressed);
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