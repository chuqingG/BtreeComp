#pragma once
#include "node_mt.h"

// Constructor of key
Key::Key(string val, int rid) {
    value = val;
    ridList.push_back(to_string(rid));
}

// Method to add new rid with same key value
void Key::addRecord(int rid) {
    ridList.push_back(to_string(rid));
}

// Method to get size of key
int Key::getSize() {
    int totalLen = value.length();
    for (int i = 0; i < ridList.size(); i++) {
        totalLen += ridList.at(i).length();
    }
    return totalLen;
}

// Key_c::Key_c(char* val, int rid){
//     shared_ptr<char[]> temp(new char[strlen(val) + 1]);
//     memcpy(temp.get(), val, strlen(val) + 1);
//     value = move(temp);
//     ridList.push_back(to_string(rid));
// }

Key_c::Key_c(char *val, int rid) {
    value = val;
    ridList.push_back(rid);
}

// Method to add new rid with same key value
void Key_c::addRecord(int rid) {
    ridList.push_back(rid);
}

// Method to get size of key
int Key_c::getSize() {
    int totalLen = strlen(value);
    totalLen += ridList.size() * sizeof(int);
    return totalLen;
}

void Key_c::update_value(string str) {
    delete value;
    char *val = new char[str.size() + 1];
    memcpy(val, str.data(), str.size() + 1);
    value = val;
}

// Constructor of Node
Node::Node() {
    size = 0;
    prefix = "";
    id = rand();
    prev = nullptr;
    next = nullptr;
    lowkey = new char[10];
    strcpy(lowkey, "");
    highkey = new char[10];
    strcpy(highkey, MAXHIGHKEY);
    level = 0;
}

// Destructor of Node
Node::~Node() {
    for (Node *childptr : ptrs) {
        delete childptr;
    }
}

void Node::lock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeLock();
        break;
    case READ:
        rwLock.readLock();
        break;
    default:
        break;
    }
}

void Node::unlock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeUnlock();
        break;
    case READ:
        rwLock.readUnlock();
        break;
    default:
        break;
    }
}

NodeDB2::NodeDB2() {
    size = 0;
    id = rand();
    prev = nullptr;
    next = nullptr;
    highkey = MAXHIGHKEY;
    level = 0;
}

// Destructor of Node
NodeDB2::~NodeDB2() {
    for (NodeDB2 *childptr : ptrs) {
        delete childptr;
    }
}

void NodeDB2::lock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeLock();
        break;
    case READ:
        rwLock.readLock();
        break;
    default:
        break;
    }
}

void NodeDB2::unlock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeUnlock();
        break;
    case READ:
        rwLock.readUnlock();
        break;
    default:
        break;
    }
}

KeyMyISAM::KeyMyISAM(string v, int p, int rid) {
    value = v;
    setPrefix(p);
    ridList.push_back(to_string(rid));
}

KeyMyISAM::KeyMyISAM(string val, int p, vector<string> list) {
    value = val;
    setPrefix(p);
    ridList = list;
}

// Method to add new rid with same key value
void KeyMyISAM::addRecord(int rid) {
    ridList.push_back(to_string(rid));
}

int KeyMyISAM::getPrefix() {
    return is1Byte ? prefix[0] : (prefix[1] << 8) | prefix[0];
}

void KeyMyISAM::setPrefix(int p) {
    if (p <= 127) {
        prefix = new u_char[1];
        prefix[0] = (u_char)(p);
        is1Byte = true;
    }
    // Prefix is 2 bytes
    else {
        prefix = new u_char[2];
        prefix[0] = (u_char)(p);
        prefix[1] = (u_char)((p >> 8));
        is1Byte = false;
    }
}

int KeyMyISAM::getSize() {
    int prefixBytes = getPrefix() <= 127 ? 1 : 2;
    int totalLen = value.length() + prefixBytes;
    for (int i = 0; i < ridList.size(); i++) {
        totalLen += ridList.at(i).length();
    }
    return totalLen;
}

NodeMyISAM::NodeMyISAM() {
    size = 0;
    id = rand();
    prev = nullptr;
    next = nullptr;
    highkey = MAXHIGHKEY;
    level = 0;
}

// Destructor of Node
NodeMyISAM::~NodeMyISAM() {
    for (NodeMyISAM *childptr : ptrs) {
        delete childptr;
    }
}

void NodeMyISAM::lock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeLock();
        break;
    case READ:
        rwLock.readLock();
        break;
    default:
        break;
    }
}

void NodeMyISAM::unlock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeUnlock();
        break;
    case READ:
        rwLock.readUnlock();
        break;
    default:
        break;
    }
}

KeyWT::KeyWT(string val, uint8_t p, int rid) {
    value = val;
    prefix = p;
    isinitialized = false;
    initialized_value = "";
    mLock = new std::shared_mutex();
    ridList.push_back(to_string(rid));
}

KeyWT::KeyWT(string val, uint8_t p, vector<string> list) {
    value = val;
    prefix = p;
    isinitialized = false;
    initialized_value = "";
    mLock = new std::shared_mutex();
    ridList = list;
}

// Method to add new rid with same key value
void KeyWT::addRecord(int rid) {
    ridList.push_back(to_string(rid));
}

// Get size of key
int KeyWT::getSize() {
    int totalLen = sizeof(prefix) + value.length();
    for (int i = 0; i < ridList.size(); i++) {
        totalLen += ridList.at(i).length();
    }
    return totalLen;
}

NodeWT::NodeWT() {
    size = 0;
    id = rand();
    prev = nullptr;
    next = nullptr;
    highkey = MAXHIGHKEY;
    level = 0;
    prefixstart = 0;
    prefixstop = 0;
}

// Destructor of Node
NodeWT::~NodeWT() {
    for (NodeWT *childptr : ptrs) {
        delete childptr;
    }
}

void NodeWT::lock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeLock();
        break;
    case READ:
        rwLock.readLock();
        break;
    default:
        break;
    }
}

void NodeWT::unlock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeUnlock();
        break;
    case READ:
        rwLock.readUnlock();
        break;
    default:
        break;
    }
}

// Constructor of Key when pKB is enabled
KeyPkB::KeyPkB(int pos, string value, char *ptr, int rid) {
    offset = pos;
    if (pos < value.length()) {
        string substr = value.substr(pos, PKB_LEN);
        strcpy(partialKey, substr.c_str());
        pkLength = substr.length();
    }
    else {
        memset(partialKey, 0, sizeof(partialKey));
        pkLength = 0;
    }
    original = ptr;
    ridList.push_back(to_string(rid));
}

// Constructor of Key when previous key is being copied
KeyPkB::KeyPkB(int pos, string value, char *ptr, vector<string> list) {
    offset = pos;
    if (pos < value.length()) {
        string substr = value.substr(pos, PKB_LEN);
        strcpy(partialKey, substr.c_str());
        pkLength = substr.length();
    }
    else {
        memset(partialKey, 0, sizeof(partialKey));
        pkLength = 0;
    }
    original = ptr;
    ridList = list;
}

// Method to add new rid with same key value
void KeyPkB::addRecord(int rid) {
    ridList.push_back(to_string(rid));
}

int KeyPkB::getSize() {
    int totalLen = sizeof(offset) + PKB_LEN + sizeof(original);
    for (int i = 0; i < ridList.size(); i++) {
        totalLen += ridList.at(i).length();
    }
    return totalLen;
}

void KeyPkB::updateOffset(int newOffset) {
    offset = newOffset;
    string str = original;
    if (newOffset < str.length()) {
        string substr = str.substr(newOffset, PKB_LEN);
        strcpy(partialKey, substr.c_str());
        pkLength = substr.length();
    }
    else {
        memset(partialKey, 0, sizeof(partialKey));
        pkLength = 0;
    }
}

NodePkB::NodePkB() {
    size = 0;
    id = rand();
    prev = nullptr;
    next = nullptr;
    lowkey = "";
    highkey = MAXHIGHKEY;
    level = 0;
}

// Destructor of Node
NodePkB::~NodePkB() {
    for (NodePkB *childptr : ptrs) {
        delete childptr;
    }
}

void NodePkB::lock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeLock();
        break;
    case READ:
        rwLock.readLock();
        break;
    default:
        break;
    }
}

void NodePkB::unlock(accessMode mode) {
    switch (mode) {
    case WRITE:
        rwLock.writeUnlock();
        break;
    case READ:
        rwLock.readUnlock();
        break;
    default:
        break;
    }
}

void printKeys(Node *node, bool compressed) {
    string prev_key = "";
    string curr_key = "";
    for (uint32_t i = 0; i < node->keys.size(); i++) {
        // Loop through rid list to print duplicates
        for (uint32_t j = 0; j < node->keys.at(i).ridList.size(); j++) {
            if (compressed) {
                cout << node->keys.at(i).value << ",";
            }
            else {
                cout << node->prefix + node->keys.at(i).value << ",";
            }
        }
    }
}

void printKeys_db2(NodeDB2 *node, bool compressed) {
    for (uint32_t i = 0; i < node->prefixMetadata.size(); i++) {
        if (compressed) {
            cout << "Prefix " << node->prefixMetadata.at(i).prefix << ": ";
        }
        for (int l = node->prefixMetadata.at(i).low;
             l <= node->prefixMetadata.at(i).high; l++) {
            // Loop through rid list to print duplicates
            for (uint32_t j = 0; j < node->keys.at(i).ridList.size(); j++) {
                if (compressed) {
                    cout << node->keys.at(l).value << ",";
                }
                else {
                    cout << node->prefixMetadata.at(i).prefix + node->keys.at(l).value << ",";
                }
            }
        }
    }
}

void printKeys_myisam(NodeMyISAM *node, bool compressed) {
    string prev_key = "";
    string curr_key = "";
    for (uint32_t i = 0; i < node->keys.size(); i++) {
        // Loop through rid list to print duplicates
        for (uint32_t j = 0; j < node->keys.at(i).ridList.size(); j++) {
            if (compressed || node->keys.at(i).getPrefix() == 0) {
                curr_key = node->keys.at(i).value;
            }
            else {
                curr_key = prev_key.substr(0, node->keys.at(i).getPrefix()) + node->keys.at(i).value;
            }
            cout << curr_key << ",";
        }
        prev_key = curr_key;
    }
}

void printKeys_wt(NodeWT *node, bool compressed) {
    string prev_key = "";
    string curr_key = "";
    for (int i = 0; i < node->keys.size(); i++) {
        // Loop through rid list to print duplicates
        for (uint32_t j = 0; j < node->keys.at(i).ridList.size(); j++) {
            if (compressed || node->keys.at(i).prefix == 0) {
                curr_key = node->keys.at(i).value;
            }
            else {
                curr_key = prev_key.substr(0, node->keys.at(i).prefix) + node->keys.at(i).value;
            }
            cout << curr_key << ",";
        }
        prev_key = curr_key;
    }
}

void printKeys_pkb(NodePkB *node, bool compressed) {
    string curr_key;
    for (int i = 0; i < node->keys.size(); i++) {
        // Loop through rid list to print duplicates
        for (uint32_t j = 0; j < node->keys.at(i).ridList.size(); j++) {
            if (compressed) {
                curr_key = node->keys.at(i).partialKey;
            }
            else {
                curr_key = node->keys.at(i).original;
            }
            cout << curr_key << ",";
        }
    }
}