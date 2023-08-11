#include "btree_std_mt_mc.h"
#include "./compression/compression_std.cpp"


// Initialise the BPTreeMT Node
BPTreeMT::BPTreeMT(int col=1, bool head_compression=false, 
                    bool tail_compression=false){
    root = new Node;
    column_num = col;
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
int BPTreeMT::search(string_view key){
    string skey;
    vector<Node *> parents;
    Node *leaf = search_leaf_node(root, key, parents);
    if (this->head_comp){
        //TODO: add colomn number
        // skey = key.substr(leaf->prefix.length());
        skey = multicol_cut_prefix(key, leaf->prefix);
    } else{
        skey = key;
    }
    int pos = leaf != nullptr ? search_binary(leaf, skey, 0, leaf->size - 1) : -1;
    // Release read lock after scanning node
    leaf->unlock(READ);
    return pos;
}

void BPTreeMT::insert(string x){
    //Lock root before writing
    lockRoot(WRITE);
    if (root->size == 0){
        root->keys.push_back(x);
        root->IS_LEAF = true;
        root->size = 1;
        root->unlock(WRITE);
        return;
    }
    vector<Node *> parents;  // access path
    Node *leaf = search_leaf_node(root, x, parents);
#ifdef MYDEBUG
    // std::cout << "want to insert " << x << " in : \n" << 
    //         leaf->prefix << ": [" << leaf->lowkey << ", " << leaf->highkey << "]" << std::endl;
#endif
    insert_leaf(leaf, parents, x);
}

// Function to peform range query on B+Tree
int BPTreeMT::searchRange(string lkey,string hkey){
    vector<Node *> parents;
    Node *leaf = search_leaf_node(root, lkey, parents);
    Node *prevleaf = nullptr;

    if(leaf!=nullptr && this->head_comp)
        lkey = multicol_cut_prefix(lkey, leaf->prefix);
    int pos = leaf != nullptr ? search_binary(leaf, lkey, 0, leaf->size - 1) : -1;
    int entries = 0;
    // Keep searching till value > max or we reach end of tree
    while (leaf != nullptr){
        if (pos == leaf->size){
            pos = 0;
            prevleaf = leaf;
            leaf = leaf->next;
            prevleaf->unlock(READ);
            if (leaf == nullptr){
                break;
            }
            leaf->lock(READ);
        }
        string leafkey = leaf->keys.at(pos);
        if(this->head_comp){
            leafkey = multicol_recover_prefix(leafkey, leaf->prefix);
        }
        if (lex_compare(leafkey, hkey) > 0){
            break;
        }
        pos++;
        entries++;
    }
    if (leaf)
        leaf->unlock(READ);
    return entries;
}

void BPTreeMT::insert_nonleaf(Node *node, vector<Node *> &parents, int pos, splitReturn childsplit){
    node->lock(WRITE);
    // Move right if any splits occurred
    node = move_right(node, childsplit.promotekey);

     // Unlock right child
    childsplit.right->unlock(WRITE);
    // Unlock prev node
    childsplit.left->unlock(WRITE);
    
    if (check_split_condition(node)){
        splitReturn currsplit = split_nonleaf(node, parents, pos, childsplit);
        if (node == root){
            Node *newRoot = new Node;
            newRoot->keys.push_back(currsplit.promotekey);
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
        if (this->head_comp) {
            promotekey = multicol_cut_prefix(promotekey, node->prefix);
            insertpos = insert_binary(node, promotekey, 0, node->size - 1);
        } else {
            insertpos = insert_binary(node, promotekey, 0, node->size - 1);
        }
        vector<string> allkeys;
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(node->keys.at(i));
        }
        allkeys.push_back(promotekey);
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

void BPTreeMT::insert_leaf(Node *leaf, vector<Node *> &parents, string key){
    // Change shared lock to exclusive
    leaf->unlock(READ);
    leaf->lock(WRITE);
    // Move right if any splits occurred
    leaf = move_right(leaf, key);
    
    if (check_split_condition(leaf)){
        splitReturn split = split_leaf(leaf, parents, key);
        if (leaf == root){
            Node *newRoot = new Node;
            newRoot->keys.push_back(split.promotekey);
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
        if (this->head_comp) {
            key = multicol_cut_prefix(key, leaf->prefix);
            // key = key.substr(leaf->prefix.length());
            insertpos = insert_binary(leaf, key, 0, leaf->size - 1);
        } else {
            insertpos = insert_binary(leaf, key, 0, leaf->size - 1);
        }
        vector<string> allkeys;
        for (int i = 0; i < insertpos; i++){
            allkeys.push_back(leaf->keys.at(i));
        }
        allkeys.push_back(key);
        for (int i = insertpos; i < leaf->size; i++){
            allkeys.push_back(leaf->keys.at(i));
        }
        leaf->keys = allkeys;
        leaf->size = leaf->size + 1;
        leaf->unlock(WRITE);
    }
}

int BPTreeMT::split_point(vector<string> allkeys)
{
    int size = allkeys.size();
    int bestsplit = size / 2;
    if (this->tail_comp){
        int split_range_low = size / 3;
        int split_range_high = 2 * size / 3;
        // Representing 16 bytes of the integer
        int minlen = INT16_MAX;
        for (int i = split_range_low; i <= split_range_high; i++)
        {
            if (i + 1 < size)
            {
                string compressed = tail_compress(allkeys.at(i),
                                                  allkeys.at(i + 1));
                if (compressed.length() < minlen)
                {
                    minlen = compressed.length();
                    bestsplit = i;
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
    string splitprefix;
    if (this->head_comp) {
        promotekey = multicol_cut_prefix(promotekey, node->prefix);
        insertpos = insert_binary(node, promotekey, 0, node->size - 1);
    } else {
        insertpos = insert_binary(node, promotekey, 0, node->size - 1);
    }
    vector<string> allkeys;
    vector<Node *> allptrs;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }
    allkeys.push_back(promotekey);
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
    string original_full_split_key = multicol_recover_prefix(allkeys.at(split), node->prefix);
    // if(original_full_split_key.length() > 32){
    //     cout << "[err]: recover prefix" << endl;
    // }
    // cout << "Best Split point " << split << " size " << allkeys.size() << endl;

    vector<string> leftkeys;
    vector<string> rightkeys;
    vector<Node *> leftptrs;
    vector<Node *> rightptrs;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split + 1, allkeys.end(), back_inserter(rightkeys));
    copy(allptrs.begin(), allptrs.begin() + split + 1, back_inserter(leftptrs));
    copy(allptrs.begin() + split + 1, allptrs.end(), back_inserter(rightptrs));

    // Lock right node
    right->lock(WRITE);

    if(this->head_comp){
        newsplit.promotekey = multicol_recover_prefix(allkeys.at(split), node->prefix);
        
        string lower_bound = node->lowkey;
        string upper_bound = node->highkey;
#ifdef MULTICOL_LHKEY_SAVE_SPACE
        //TODO: 
        string leftprefix = find_multicol_head_comp_prefix(lower_bound, newsplit.promotekey);
#else   
        string leftprefix = find_multicol_head_comp_prefix(lower_bound, original_full_split_key);
#endif
        leftkeys = update_prefix_on_keys(leftkeys, prevprefix, leftprefix);
        node->prefix = leftprefix;
#ifdef MYDEBUG
        std::cout << "[PREV]: " << lower_bound << ", " << original_full_split_key << std::endl;
        std::cout << ">>>" << leftprefix << std::endl;
#endif
        if(upper_bound.compare(MAXHIGHKEY) != 0){
            // not rightmost node
#ifdef MULTICOL_LHKEY_SAVE_SPACE
        //TODO: 
            string rightprefix = find_multicol_head_comp_prefix(newsplit.promotekey, upper_bound);
#else   
            string rightprefix = find_multicol_head_comp_prefix(original_full_split_key, upper_bound);
#endif
            rightkeys = update_prefix_on_keys(rightkeys, prevprefix, rightprefix);
            right->prefix = rightprefix;
#ifdef MYDEBUG
            std::cout << "[PREV]: " << original_full_split_key << ", " << upper_bound << std::endl;
            std::cout << ">>>" << rightprefix << std::endl;
#endif
        }
        else{
            right->prefix = prevprefix;
        }
    } else {
        newsplit.promotekey = allkeys.at(split);
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
    right->highkey = node->highkey;

    if(this->head_comp){

#ifdef MULTICOL_LHKEY_SAVE_SPACE
        node->highkey = newsplit.promotekey;
        right->lowkey = newsplit.promotekey;
#else
        node->highkey = original_full_split_key;
        right->lowkey = original_full_split_key;
#endif
    } else {
        node->highkey = newsplit.promotekey;
        right->lowkey = newsplit.promotekey;
    }

    // Set next pointers
    Node *next = node->next;
    if (next)
        next->lock(WRITE);
    right->next = next;
    node->next = right;
    if (next)
        next->unlock(WRITE);

    newsplit.left = node;
    newsplit.right = right;

    return newsplit;
}

splitReturn BPTreeMT::split_leaf(Node *node, vector<Node *> &parents, string newkey){
    splitReturn newsplit;
    Node *right = new Node;
    string prevprefix = node->prefix;
    int insertpos;
    if (this->head_comp) {
        string comp_key = multicol_cut_prefix(newkey, node->prefix);
        // newkey = newkey.substr(node->prefix.length());
        insertpos = insert_binary(node, comp_key, 0, node->size - 1);
    } else {
        insertpos = insert_binary(node, newkey, 0, node->size - 1);
    }

    vector<string> allkeys;

    for (int i = 0; i < insertpos; i++){
        allkeys.push_back(node->keys.at(i));
    }
    allkeys.push_back(newkey);
    for (int i = insertpos; i < node->size; i++){
        allkeys.push_back(node->keys.at(i));
    }

    int split = split_point(allkeys);
    string original_full_split_key;

    if (split == insertpos){
        // don't need to recover the split key 
        original_full_split_key = newkey;
    } else{
        original_full_split_key = multicol_recover_prefix(allkeys.at(split), node->prefix);
    }

#ifdef MYDEBUG
    std::cout << node->prefix << " + " << allkeys.at(split) << std::endl;
    std::cout << "==> " << original_full_split_key << std:: endl;
#endif

    vector<string> leftkeys;
    vector<string> rightkeys;
    copy(allkeys.begin(), allkeys.begin() + split, back_inserter(leftkeys));
    copy(allkeys.begin() + split, allkeys.end(), back_inserter(rightkeys));
    
    string firstright;
    string lastleft;
    if (this->tail_comp) {
        lastleft = leftkeys.at(leftkeys.size() - 1);
        firstright = rightkeys.at(0);
        if (this->head_comp) {
            lastleft = multicol_recover_prefix(lastleft, node->prefix);
            firstright = original_full_split_key;
        } 
        newsplit.promotekey = tail_compress(lastleft, firstright);
    } else {
        firstright = rightkeys.at(0);
        if (this->head_comp) {
            firstright = original_full_split_key;
        } 
        newsplit.promotekey = firstright;
    }

    // Lock right node
    right->lock(WRITE);

    // update the compressed keys
    if (this->head_comp) {
        // string lower_bound, upper_bound;
        // TODO: if MULTICOL_LHKEY_SAVE_SPACE ?
        // if(this->tail_comp){
        //     lower_bound = node->keys.at(0);
        //     if(node->highkey.compare(MAXHIGHKEY) != 0){
                
        //     } else {
        //         // rightmost
        //         upper_bound = node->highkey;
        //     }
        // } else{
        //     lower_bound = node->lowkey;
        //     upper_bound = node->highkey;
        // }
#ifdef MULTICOL_LHKEY_SAVE_SPACE
        // TODO: 
#else   
        string lower_bound = node->lowkey;
        string upper_bound = node->highkey;
        string leftprefix = find_multicol_head_comp_prefix(lower_bound, newsplit.promotekey);
#endif
        leftkeys = update_prefix_on_keys(leftkeys, prevprefix, leftprefix);
        node->prefix = leftprefix;
#ifdef MYDEBUG
        std::cout << "[PREV]: " << lower_bound << ", " << original_full_split_key << std::endl;
        std::cout << ">>>" << leftprefix << std::endl;
#endif
        if(upper_bound.compare(MAXHIGHKEY) != 0){
            // not rightmost node
            string rightprefix = find_multicol_head_comp_prefix(newsplit.promotekey, upper_bound);
            rightkeys = update_prefix_on_keys(rightkeys, prevprefix, rightprefix);
            right->prefix = rightprefix;
#ifdef MYDEBUG
            std::cout << "[PREV]: " << original_full_split_key << ", " << upper_bound << std::endl;
            std::cout << ">>>" << rightprefix << std::endl;
#endif
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
    right->highkey = node->highkey;
    
    if(this->head_comp){

#ifdef MULTICOL_LHKEY_SAVE_SPACE
        node->highkey = newsplit.promotekey;
        right->lowkey = newsplit.promotekey;
#else
        node->highkey = original_full_split_key;
        right->lowkey = original_full_split_key;
#endif
    } else {
        node->highkey = newsplit.promotekey;
        right->lowkey = newsplit.promotekey;
    }

    // Set next pointers
    Node *next = node->next;
    if (next)
        next->lock(WRITE);
    right->next = next;
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
        currspace += node->keys.at(i).length();
    }
    return node->size > 1 && currspace >= MAX_SIZE_IN_BYTES;
#else
    return node->size == MAX_NODE_SIZE;
#endif
}

int BPTreeMT::insert_binary(Node *cursor, string_view key, int low, int high){
    while (low <= high){
        int mid = low + (high - low) / 2;
        int cmp = lex_compare(string(key), cursor->keys.at(mid));
        if (cmp == 0)
            return mid + 1;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return high + 1;
}

Node* BPTreeMT::search_leaf_node(Node *root, string_view key, vector<Node *> &parents)
{   
    // called when insertion
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
        string_view searchkey = key;
         // Acquire read lock before scanning node
        Node *nextPtr = scan_node(cursor, searchkey);
        if (nextPtr != cursor->next){
            // find successfully
            parents.push_back(cursor);
        }
#ifdef MYDEBUG
        if(nextPtr->lowkey.compare(key) > 0){
            std::cout << "[err]: find leaf" << std::endl;
        }
#endif
         // Release read lock after scanning node
        cursor->unlock(READ);
        // Acquire lock on next ptr
        nextPtr->lock(READ);
        cursor = nextPtr;
    }
    return cursor;
}

int BPTreeMT::search_binary(Node *cursor, string_view key, int low, int high){
    while (low <= high){
        int mid = low + (high - low) / 2;
        int cmp = lex_compare(string(key), cursor->keys.at(mid));
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}


Node* BPTreeMT::scan_node(Node *node, string_view key){
    if (node->highkey.compare(MAXHIGHKEY) != 0 && node->highkey.compare(key) <= 0)
    {
        return node->next;
    }
    else if (node->IS_LEAF)
        return node;
    else{
        string searchkey;
        int pos;
        if(this->head_comp){
            searchkey = multicol_cut_prefix(key, node->prefix);
#ifdef MYDEBUG
            // std::cout << "Cut (" << key << ") to (" << searchkey << ") by " << node->prefix << std::endl; 
#endif
            // searchkey = key.substr(node->prefix.length());
            pos = insert_binary(node, searchkey, 0, node->size - 1);
        } else{
            pos = insert_binary(node, key, 0, node->size - 1);
        }
        
#ifdef MYDEBUG
        std::cout << "insert pos: " << pos << std::endl; 
        if(node->ptrs.at(pos)->lowkey.compare(key) > 0){
            std::cout << "whywhywhy" << std::endl;
        }
#endif
        return node->ptrs.at(pos);
    }
}

Node* BPTreeMT::move_right(Node *node, string key)
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

    string key = currRoot->keys.at(0);
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
            currSize += cursor->keys.at(i).length();
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