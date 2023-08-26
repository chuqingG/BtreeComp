#include "btree_pkb_mt.h"

// Initialise the BPTreePkBMT NodePkB
BPTreePkBMT::BPTreePkBMT(){
    root = new NodePkB;
}

// Destructor of BPTreePkBMT tree
BPTreePkBMT::~BPTreePkBMT(){
    delete root;
}

// Function to get the root NodePkB
NodePkB *BPTreePkBMT::getRoot(){
    return root;
}

// Function to set the root of BPTreeDB2
void BPTreePkBMT::setRoot(NodePkB *newRoot)
{
    root = newRoot;
}

void BPTreePkBMT::lockRoot(accessMode mode)
{
    // If root changed in the middle, update to original root
    NodePkB *prevRoot = root;
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
int BPTreePkBMT::search(string_view key){
    vector<NodePkB *> parents;
    int offset = 0;
    bool equal = false;
    NodePkB *leaf = search_leaf_node(root, key, parents, offset);
    int pos = -1;
    if (leaf != nullptr) {
        findNodeResult result = find_node(leaf, string(key), offset, equal);
        if (result.low == result.high)
            pos = result.low;
    }
    leaf->unlock(READ);
    return pos;
}

void BPTreePkBMT::insert(string x){
    lockRoot(WRITE);
    auto it = strPtrMap.find(x);
    if (root->size == 0){
        int rid = rand();
        root->keys.push_back(KeyPkB(0, x, it->second, rid));
        root->IS_LEAF = true;
        root->size = 1;
        root->unlock(WRITE);
        return;
    }
    vector<NodePkB *> parents;
    int offset = 0;
    NodePkB *leaf = search_leaf_node(root, x, parents, offset);
    insert_leaf(leaf, parents, x, it->second, offset);
}

void BPTreePkBMT::insert_nonleaf(NodePkB *node, vector<NodePkB *> &parents, int pos, splitReturnPkB childsplit){
    int offset = 0;
    node->lock(WRITE);
    // Move right if any splits occurred
    node = move_right(node, childsplit.promotekey, offset);

    // Unlock right child
    childsplit.right->unlock(WRITE);
    // Unlock prev node
    childsplit.left->unlock(WRITE);
    
    string basekey = node->lowkey;
    offset = compute_offset(basekey, childsplit.promotekey);

    if (check_split_condition(node)){
        splitReturnPkB currsplit = split_nonleaf(node, parents, pos, childsplit, offset);
        if (node == root){
            NodePkB *newRoot = new NodePkB;
            int rid = rand();
            newRoot->keys.push_back(KeyPkB(0, currsplit.promotekey, currsplit.keyptr, rid));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            newRoot->level = node->level + 1;
            setRoot(newRoot);
            currsplit.right->unlock(WRITE);
            currsplit.left->unlock(WRITE);
        }else{
            NodePkB *parent = nullptr;
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
        int rid = rand();
        bool equal = false;
        findNodeResult result = find_node(node, promotekey, offset, equal);
        insertpos = result.high;
        vector<KeyPkB> allkeys;
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        KeyPkB pkbKey = generate_pkb_key(node, promotekey, childsplit.keyptr, insertpos, parents, pos, rid);
        allkeys.push_back(pkbKey);

        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(node->keys.at(i));
        }

        vector<NodePkB *> allptrs;
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

        build_page_prefixes(node, insertpos); // Populate new prefixes

        node->unlock(WRITE);
    }
}

void BPTreePkBMT::insert_leaf(NodePkB *leaf, vector<NodePkB *> &parents, string key, char *keyptr, int offset){
    // Change shared lock to exclusive
    leaf->unlock(READ);
    leaf->lock(WRITE);
    // Move right if any splits occurred
    leaf = move_right(leaf, key, offset);

    if (check_split_condition(leaf)){
        splitReturnPkB split = split_leaf(leaf, parents, key, keyptr, offset);
        if (leaf == root){
            NodePkB *newRoot = new NodePkB;
            int rid = rand();
            newRoot->keys.push_back(KeyPkB(0, split.promotekey, split.keyptr, rid));
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
            NodePkB *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }else{
        int insertpos;
        int rid = rand();
        bool equal = false;
        findNodeResult result = find_node(leaf, key, offset, equal);
        insertpos = result.high;

        vector<KeyPkB> allkeys;
        if (equal) {
            allkeys = leaf->keys;
            allkeys.at(insertpos).addRecord(rid);
        }
        else {
            for (int i = 0; i < insertpos; i++){
                allkeys.push_back(leaf->keys.at(i));
            }

            KeyPkB pkbKey = generate_pkb_key(leaf, key, keyptr, insertpos, parents, parents.size() - 1, rid);
            allkeys.push_back(pkbKey);
            for (int i = insertpos; i < leaf->size; i++){
                allkeys.push_back(leaf->keys.at(i));
            }
        }
        leaf->keys = allkeys;
        if (!equal) {
            leaf->size = leaf->size + 1;
            build_page_prefixes(leaf, insertpos); // Populate new prefixes
        }

        leaf->unlock(WRITE);
    }
}

int BPTreePkBMT::split_point(vector<KeyPkB> allkeys){
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

uncompressedKey BPTreePkBMT::get_uncompressed_key_before_insert(NodePkB *node, int ind, int insertpos, string newkey, char *newkeyptr, bool equal){
    uncompressedKey uncompressed;
    if (equal) {
        uncompressed.key = get_key(node, ind);
        uncompressed.keyptr = node->keys.at(ind).original;
        return uncompressed;
    }
    if (insertpos == ind){
        uncompressed.key = newkey;
        uncompressed.keyptr = newkeyptr;
    }else{
        int origind = insertpos < ind ? ind - 1 : ind;
        uncompressed.key = get_key(node, origind);
        uncompressed.keyptr = node->keys.at(origind).original;
    }
    return uncompressed;
}

splitReturnPkB BPTreePkBMT::split_nonleaf(NodePkB *node, vector<NodePkB *> parents, int pos, splitReturnPkB childsplit, int offset){
    splitReturnPkB newsplit;
    NodePkB *right = new NodePkB;
    string promotekey = childsplit.promotekey;
    char *promotekeyptr = childsplit.keyptr;
    int insertpos;
    int rid = rand();
    bool equal = false;
    findNodeResult result = find_node(node, promotekey, offset, equal);
    insertpos = result.high;

    vector<KeyPkB> allkeys;
    vector<NodePkB *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }
    KeyPkB pkbKey = generate_pkb_key(node, promotekey, promotekeyptr, insertpos, parents, pos, rid);
    allkeys.push_back(pkbKey);

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

    vector<KeyPkB> leftkeys;
    vector<KeyPkB> rightkeys;
    vector<NodePkB *> leftptrs;
    vector<NodePkB *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    string splitkey;

    uncompressedKey firstrightKeyAndPtr = get_uncompressed_key_before_insert(node, split + 1, insertpos, promotekey, promotekeyptr, equal);
    uncompressedKey splitKeyAndPtr = get_uncompressed_key_before_insert(node, split, insertpos, promotekey, promotekeyptr, equal);
    splitkey = splitKeyAndPtr.key;
    newsplit.keyptr = splitKeyAndPtr.keyptr;
    int firstrightoffset = compute_offset(splitkey, firstrightKeyAndPtr.key);
    rightkeys.at(0) = KeyPkB(firstrightoffset, firstrightKeyAndPtr.key, firstrightKeyAndPtr.keyptr, rightkeys.at(0).ridList);

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
    right->lowkey = newsplit.promotekey;
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;

    // Set next pointers
    NodePkB *next = node->next;
    right->prev = node;
    if (next)
        next->lock(WRITE);
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;
    if (next)
        next->unlock(WRITE);
    
    if (insertpos < split){
        build_page_prefixes(node, insertpos); // Populate new prefixes
    }
    build_page_prefixes(right, 0); // Populate new prefixes

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturnPkB BPTreePkBMT::split_leaf(NodePkB *node, vector<NodePkB *> &parents, string newkey, char *newkeyptr, int offset){
    splitReturnPkB newsplit;
    NodePkB *right = new NodePkB;
    int insertpos;
    int rid = rand();
    bool equal = false;

    findNodeResult result = find_node(node, newkey, offset, equal);
    insertpos = result.high;

    vector<KeyPkB> allkeys;
    if (equal) {
        allkeys = node->keys;
        allkeys.at(insertpos).addRecord(rid);
    }
    else {
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        KeyPkB pkbKey = generate_pkb_key(node, newkey, newkeyptr, insertpos, parents, parents.size() - 1, rid);
        allkeys.push_back(pkbKey);

        for (int i = insertpos; i < node->size; i++){
            allkeys.push_back(node->keys.at(i));
        }
    }

    int split = split_point(allkeys);

    vector<KeyPkB> leftkeys;
    vector<KeyPkB> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));

    string firstright;
    uncompressedKey firstrightKeyAndPtr = get_uncompressed_key_before_insert(node, split, insertpos, newkey, newkeyptr, equal);
    firstright = firstrightKeyAndPtr.key;
    rightkeys.at(0) = KeyPkB(firstright.length(), firstright, firstrightKeyAndPtr.keyptr, rightkeys.at(0).ridList);
    newsplit.keyptr = firstrightKeyAndPtr.keyptr;
    
    newsplit.promotekey = firstright;

    node->size = leftkeys.size();
    node->keys = leftkeys;

    // Lock right node
    right->lock(WRITE);

    right->size = rightkeys.size();
    right->IS_LEAF = true;
    right->keys = rightkeys;
    right->level = node->level;

     // Set high and low keys
    right->lowkey = newsplit.promotekey;
    right->highkey = node->highkey;
    node->highkey = newsplit.promotekey;

    // Set next pointers
    NodePkB *next = node->next;
    right->prev = node;
    if (next)
        next->lock(WRITE);
    right->next = next;
    if (next)
        next->prev = right;
    node->next = right;
    if (next)
        next->unlock(WRITE);
    
    if (!equal  && insertpos < split){
        build_page_prefixes(node, insertpos); // Populate new prefixes
    }
    build_page_prefixes(right, 0); // Populate new prefixes

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreePkBMT::check_split_condition(NodePkB *node){
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

NodePkB* BPTreePkBMT::search_leaf_node(NodePkB *root, string_view key, vector<NodePkB *> &parents, int &offset)
{
    // Tree is empty
    if (root->size == 0){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodePkB *cursor = root;
    cursor->unlock(WRITE);
    cursor->lock(READ);

    // Till we reach leaf node
    while (!cursor->IS_LEAF){
        string_view searchkey = key;
        // Acquire read lock before scanning node
        NodePkB *nextPtr = scan_node(cursor, string(searchkey), offset);
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

NodePkB* BPTreePkBMT::scan_node(NodePkB *node, string key, int &offset){
    if (node->highkey.compare(MAXHIGHKEY) != 0 && node->highkey.compare(key) <= 0)
    {
        NodePkB *next =  node->next;
        //Need to change offset as base key changes with node
        if(next != nullptr)
            offset = compare_key(key , next->lowkey).offset;
        return next;
    }
    else if (node->IS_LEAF)
        return node;
    else
    {
        int pos;
        bool equal = false;
        findNodeResult result = find_node(node, key, offset, equal);
        // If equal key found, choose next position
        if (result.low == result.high)
            pos = result.high + 1;
        else
            pos = result.high;
        offset = result.offset;
        return node->ptrs.at(pos);
    }
}

NodePkB* BPTreePkBMT::move_right(NodePkB *node, string key, int &offset)
{
    NodePkB *current = node;
    NodePkB *nextPtr;
    while ((nextPtr = scan_node(current, string(key), offset)) == current->next)
    {
        current->unlock(WRITE);
        nextPtr->lock(WRITE);
        current = nextPtr;
    }

    return current;
}

//Fetch root if it was changed by concurrent threads
void BPTreePkBMT::bt_fetch_root(NodePkB *currRoot, vector<NodePkB *> &parents){
    NodePkB *cursor = root;
    cursor->lock(READ);
    if (cursor->level == currRoot->level)
    {
        cursor->unlock(READ);
        return;
    }

    //Get decompressed value to find changed root
    string key = currRoot->keys.at(0).original;
    NodePkB *nextPtr;
    int offset = 0;

    while (cursor->level != currRoot->level)
    {
        NodePkB *nextPtr = scan_node(cursor, key, offset);
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

void BPTreePkBMT::getSize(NodePkB *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
                     int &totalBranching, unsigned long &totalKeySize, int &totalPrefixSize){
    if (cursor != NULL){
        unsigned long currSize = 0;
        int prefixSize = 0;
        // One time computation
        if (cursor == root){
          for (auto it = strPtrMap.begin(); it != strPtrMap.end(); it++){
            currSize += it->first.length();
          }
        }
        for (int i = 0; i < cursor->size; i++){
          currSize += cursor->keys.at(i).getSize();
          prefixSize += sizeof(cursor->keys.at(i).offset);
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

int BPTreePkBMT::getHeight(NodePkB *cursor){
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

void BPTreePkBMT::printTree(NodePkB *x, vector<bool> flag, bool compressed, int depth, bool isLast)
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
        printKeys_pkb(x, compressed);
        cout << endl;
    }

    // Condition when the node is
    // the last node of
    // the exploring depth
    else if (isLast){
        cout << "+--- ";
        printKeys_pkb(x, compressed);
        cout << endl;

        // No more childrens turn it
        // to the non-exploring depth
        flag[depth] = false;
    }else{
        cout << "+--- ";
        printKeys_pkb(x, compressed);
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