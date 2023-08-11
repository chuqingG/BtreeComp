#include "btree_myisam_mt_mc.h"
#include "./compression/compression_myisam.cpp"


// Initialise the BPTreeMyISAMMT NodeMyISAM
BPTreeMyISAMMT::BPTreeMyISAMMT(bool non_leaf_compression=false){
    root = new NodeMyISAM;
    non_leaf_comp = non_leaf_compression;
}

// Destructor of BPTreeMyISAMMT tree
BPTreeMyISAMMT::~BPTreeMyISAMMT(){
    delete root;
}

// Function to get the root NodeMyISAM
NodeMyISAM *BPTreeMyISAMMT::getRoot(){
    return root;
}

// Function to set the root of BPTreeDB2
void BPTreeMyISAMMT::setRoot(NodeMyISAM *newRoot)
{
    root = newRoot;
}

void BPTreeMyISAMMT::lockRoot(accessMode mode)
{
    // If root changed in the middle, update to original root
    NodeMyISAM *prevRoot = root;
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
int BPTreeMyISAMMT::search(string_view key){
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, key, parents);
    int pos = leaf != nullptr ?  prefix_search(leaf, key) : -1;
    leaf->unlock(READ);
    return pos;
}

// Function to peform range query on B+Tree
int BPTreeMyISAMMT::searchRange(string min, string max){
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, min, parents);
    NodeMyISAM *prevleaf = nullptr;
    int pos = leaf != nullptr ?  prefix_search(leaf, min) : -1;
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

        string leafkey = prevkey.substr(0, leaf->keys.at(pos).getPrefix()) + leaf->keys.at(pos).value;
        if (lex_compare(leafkey, max) > 0)
        {
            break;
        }
        prevkey = leafkey;
        pos++;
        entries++;
    }
    if (leaf)
        leaf->unlock(READ);
    return entries;
}

void BPTreeMyISAMMT::insert(string x){
    lockRoot(WRITE);
    if (root->size == 0) {
        root->keys.push_back(keyMyISAM(x, 0));
        root->IS_LEAF = true;
        root->size = 1;
        root->unlock(WRITE);
        return;
    }
    vector<NodeMyISAM *> parents;
    NodeMyISAM *leaf = search_leaf_node(root, x, parents);
    insert_leaf(leaf, parents, x);
}


/* Adapted from MyISAM code base
   Based on the implementation of _mi_prefix_search in
   {https://github.com/mysql/mysql-server/blob/a246bad76b9271cb4333634e954040a970222e0a/storage/myisam/mi_search.cc#L277}
*/
int BPTreeMyISAMMT::prefix_search(NodeMyISAM *cursor, string_view key){
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

int BPTreeMyISAMMT::prefix_insert(NodeMyISAM *cursor, string_view key){
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
                if (my_flag == 0)
                    return ind + 1;
            }
            matched -= left;
        }

        ind++;
    }
    return cursor->size;
}

void BPTreeMyISAMMT::insert_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, int pos, splitReturnMyISAM childsplit){
    node->lock(WRITE);
    // Move right if any splits occurred
    node = move_right(node, childsplit.promotekey);

    // Unlock right child
    childsplit.right->unlock(WRITE);
    // Unlock prev node
    childsplit.left->unlock(WRITE);

    
    if (check_split_condition(node)){       
        splitReturnMyISAM currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            NodeMyISAM *newRoot = new NodeMyISAM;
            newRoot->keys.push_back(keyMyISAM(currsplit.promotekey, 0));
            newRoot->ptrs.push_back(currsplit.left);
            newRoot->ptrs.push_back(currsplit.right);
            newRoot->IS_LEAF = false;
            newRoot->size = 1;
            newRoot->level = node->level + 1;
            setRoot(newRoot);
            currsplit.right->unlock(WRITE);
            currsplit.left->unlock(WRITE);
        }else {
            NodeMyISAM *parent = nullptr;
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
        if (this->non_leaf_comp) {
            insertpos = prefix_insert(node, promotekey);
        } else {
            insertpos = insert_binary(node, promotekey, 0, node->size - 1);
        }
        vector<keyMyISAM> allkeys;
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        if(this->non_leaf_comp){
            if (insertpos > 0){
                string prev_key = get_key(node, insertpos - 1);
                int pfx = compute_prefix_myisam(prev_key, promotekey);
                string compressed = promotekey.substr(pfx);
                allkeys.push_back(keyMyISAM(compressed, pfx));
            }else{
                allkeys.push_back(keyMyISAM(promotekey, 0));
            }
        }else{
            allkeys.push_back(keyMyISAM(promotekey, 0));
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
        
        node->unlock(WRITE);
    }
}

void BPTreeMyISAMMT::insert_leaf(NodeMyISAM *leaf, vector<NodeMyISAM *> &parents, string key){
    // Change shared lock to exclusive
    leaf->unlock(READ);
    leaf->lock(WRITE);
    // Move right if any splits occurred
    leaf = move_right(leaf, key);

    if (check_split_condition(leaf)){
        splitReturnMyISAM split = split_leaf(leaf, parents, key);
        if (leaf == root){
            NodeMyISAM *newRoot = new NodeMyISAM;
            newRoot->keys.push_back(keyMyISAM(split.promotekey, 0));
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
            NodeMyISAM *parent = parents.at(parents.size() - 1);
            insert_nonleaf(parent, parents, parents.size() - 2, split);
        }
    }else{
        int insertpos;
        insertpos = prefix_insert(leaf, key);
        vector<keyMyISAM> allkeys;
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(leaf->keys.at(i));
        }
        if (insertpos > 0){
            string prev_key = get_key(leaf, insertpos - 1);
            int pfx = compute_prefix_myisam(prev_key, key);
            string compressed = key.substr(pfx);
            allkeys.push_back(keyMyISAM(compressed, pfx));
        }else{
            allkeys.push_back(keyMyISAM(key, 0));
        }

        for (int i = insertpos; i < leaf->size; i++){
            allkeys.push_back(leaf->keys.at(i));
        }
        leaf->keys = allkeys;
        leaf->size = leaf->size + 1;

        // Rebuild prefixes if a new item is inserted in position 0
        build_page_prefixes(leaf, insertpos, key); // Populate new prefixes

        leaf->unlock(WRITE);
    }
}

int BPTreeMyISAMMT::split_point(vector<keyMyISAM> allkeys){
    int size = allkeys.size();
    int bestsplit = size / 2;
    return bestsplit;
}

splitReturnMyISAM BPTreeMyISAMMT::split_nonleaf(NodeMyISAM *node, vector<NodeMyISAM *> parents, int pos, splitReturnMyISAM childsplit){
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;
    string promotekey = childsplit.promotekey;
    int insertpos;
    string splitprefix;
    if (this->non_leaf_comp) {
        insertpos = prefix_insert(node, promotekey);
    } else {
        insertpos = insert_binary(node, promotekey, 0, node->size - 1);
    }

    vector<keyMyISAM> allkeys;
    vector<NodeMyISAM *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }
    if (this->non_leaf_comp){
        if (insertpos > 0){
            string prev_key = get_key(node, insertpos - 1);
            int pfx = compute_prefix_myisam(prev_key, promotekey);
            string compressed = promotekey.substr(pfx);
            allkeys.push_back(keyMyISAM(compressed, pfx));
        } else{
            allkeys.push_back(keyMyISAM(promotekey, 0));
        }
    } else {
        allkeys.push_back(keyMyISAM(promotekey, 0));
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

    vector<keyMyISAM> leftkeys;
    vector<keyMyISAM> rightkeys;
    vector<NodeMyISAM *> leftptrs;
    vector<NodeMyISAM *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    string splitkey;
    string firstright;

    if(this->non_leaf_comp){
        firstright = get_uncompressed_key_before_insert(node, split + 1, insertpos, promotekey);
        rightkeys.at(0) = keyMyISAM(firstright, 0);
        splitkey = get_uncompressed_key_before_insert(node, split, insertpos, promotekey);
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
    NodeMyISAM *next = node->next;
    if (next)
        next->lock(WRITE);
    right->next = next;
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

splitReturnMyISAM BPTreeMyISAMMT::split_leaf(NodeMyISAM *node, vector<NodeMyISAM *> &parents, string newkey){
    splitReturnMyISAM newsplit;
    NodeMyISAM *right = new NodeMyISAM;
    int insertpos = prefix_insert(node, newkey);

    vector<keyMyISAM> allkeys;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }

    if (insertpos > 0){
        string prev_key = get_key(node, insertpos - 1);
        int pfx = compute_prefix_myisam(prev_key, newkey);
        string compressed = newkey.substr(pfx);
        allkeys.push_back(keyMyISAM(compressed, pfx));
    } else{
        allkeys.push_back(keyMyISAM(newkey, 0));
    }
    
    for (int i = insertpos; i < node->size; i++){
        allkeys.push_back(node->keys.at(i));
    }

    int split = split_point(allkeys);

    vector<keyMyISAM> leftkeys;
    vector<keyMyISAM> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));

    string firstright;
    firstright = get_uncompressed_key_before_insert(node, split, insertpos, newkey);
    rightkeys.at(0) = keyMyISAM(firstright, 0);
    newsplit.promotekey = firstright;

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
    NodeMyISAM *next = node->next;
    if (next)
        next->lock(WRITE);
    right->next = next;
    node->next = right;
    if (next)
        next->unlock(WRITE);

    if (insertpos < split){
        build_page_prefixes(node, insertpos, newkey); // Populate new prefixes
    }
    build_page_prefixes(right, 0, firstright); // Populate new prefixes

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

bool BPTreeMyISAMMT::check_split_condition(NodeMyISAM *node){
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

int BPTreeMyISAMMT::insert_binary(NodeMyISAM *cursor, string_view key, int low, int high){
    while (low <= high){
        int mid = low + (high - low) / 2;
        string pagekey = cursor->keys.at(mid).value;
        int cmp = lex_compare(string(key), cursor->keys.at(mid).value);
        if (cmp == 0)
            return mid + 1;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return high + 1;
}

NodeMyISAM* BPTreeMyISAMMT::search_leaf_node(NodeMyISAM *root, string_view key, vector<NodeMyISAM *> &parents)
{
    // Tree is empty
    if (root->size == 0){
        cout << "Tree is empty" << endl;
        return nullptr;
    }

    NodeMyISAM *cursor = root;
    cursor->unlock(WRITE);
    cursor->lock(READ);

    // Till we reach leaf node
    while (!cursor->IS_LEAF) {
        string_view searchkey = key;
         // Acquire read lock before scanning node
        NodeMyISAM *nextPtr = scan_node(cursor, string(searchkey));
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

int BPTreeMyISAMMT::search_binary(NodeMyISAM *cursor, string_view key, int low, int high){
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

NodeMyISAM* BPTreeMyISAMMT::scan_node(NodeMyISAM *node, string key){
    if (node->highkey.compare(MAXHIGHKEY) != 0 && node->highkey.compare(key) <= 0)
    {
        return node->next;
    }
    else if (node->IS_LEAF)
        return node;
    else
    {
        int pos = prefix_insert(node, key);
        return node->ptrs.at(pos);
    }
}

NodeMyISAM* BPTreeMyISAMMT::move_right(NodeMyISAM *node, string key)
{
    NodeMyISAM *current = node;
    NodeMyISAM *nextPtr;
    while ((nextPtr = scan_node(current, string(key))) == current->next)
    {
        current->unlock(WRITE);
        nextPtr->lock(WRITE);
        current = nextPtr;
    }

    return current;
}

//Fetch root if it was changed by concurrent threads
void BPTreeMyISAMMT::bt_fetch_root(NodeMyISAM *currRoot, vector<NodeMyISAM *> &parents){
    NodeMyISAM *cursor = root;

    cursor->lock(READ);
    if (cursor->level == currRoot->level)
    {
        cursor->unlock(READ);
        return;
    }

    string key = currRoot->keys.at(0).value;
    NodeMyISAM *nextPtr;

    while (cursor->level != currRoot->level)
    {
        NodeMyISAM *nextPtr = scan_node(cursor, key);
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

void BPTreeMyISAMMT::getSize(NodeMyISAM *cursor, int &numNodes, int &numNonLeaf, int &numKeys,
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

int BPTreeMyISAMMT::getHeight(NodeMyISAM *cursor){
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

void BPTreeMyISAMMT::printTree(NodeMyISAM *x, vector<bool> flag, bool compressed, int depth, bool isLast)
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