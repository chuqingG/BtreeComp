#pragma once
#include "node.h"

Data::Data(const char *s) {
    // addr_ = s;

    size = strlen(s);
    addr_ = new char[size + 1];
    strcpy(addr_, s);
}

Data::Data(const char *s, int len) {
    addr_ = new char[len + 1];
    strcpy(addr_, s);
    size = len;
}

Data::Data(const std::string &s) {
    // addr_ = s.data();
    size = s.size();
    addr_ = new char[size + 1];
    strcpy(addr_, s.data());
}

const char *Data::addr() {
    return addr_;
}

Data::~Data() {
    delete addr_;
}

// void Data::set(const char *p, int s) {
//     addr_ = p;
//     size = s;
// }

// Data::~Data()
// {
//     if (size)
//         delete ptr;
// }

// void Data::from_string(string s)
// {
//     size = s.size();
//     ptr = new char[size + 1];
//     strcpy(ptr, s.data());
//     return;
// }

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
Node::Node(bool tail_comp) {
    size = 0;
    prefix = new Data("");
    base = NewPage();
    memusage = 0;
    prev = nullptr;
    next = nullptr;
    lowkey = new Data("");
    highkey = new Data("infinity");
    ptr_cnt = 0;
    // ptrs = new Node*[kNumberBound + 1];
    keys_size = new uint8_t[kNumberBound + tail_comp * TAIL_COMP_RELAX];
    std::memset(keys_size, 0, 
                (kNumberBound + tail_comp * TAIL_COMP_RELAX) * sizeof(uint8_t));
    keys_offset = new uint16_t[kNumberBound + tail_comp * TAIL_COMP_RELAX];
}

// Destructor of Node
Node::~Node() {
    // delete ptrs;
    delete keys_offset;
    delete keys_size;
    delete base;
}

DB2Node::DB2Node() {
    size = 0;
    prev = nullptr;
    next = nullptr;
}

// Destructor of DB2Node
DB2Node::~DB2Node() {
    for (DB2Node *childptr : ptrs) {
        delete childptr;
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
    prev = nullptr;
    next = nullptr;
}

// Destructor of NodeMyISAM
NodeMyISAM::~NodeMyISAM() {
    for (NodeMyISAM *childptr : ptrs) {
        delete childptr;
    }
}

KeyWT::KeyWT(string val, uint8_t p, int rid) {
    value = val;
    prefix = p;
    isinitialized = false;
    initialized_value = "";
    ridList.push_back(to_string(rid));
}

KeyWT::KeyWT(string val, uint8_t p, vector<string> list) {
    value = val;
    prefix = p;
    isinitialized = false;
    initialized_value = "";
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
    prev = nullptr;
    next = nullptr;
    prefixstart = 0;
    prefixstop = 0;
}

// Destructor of NodeWT
NodeWT::~NodeWT() {
    for (NodeWT *childptr : ptrs) {
        delete childptr;
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
    prev = nullptr;
    next = nullptr;
}

// Destructor of NodePkB
NodePkB::~NodePkB() {
    for (NodePkB *childptr : ptrs) {
        delete childptr;
    }
}

#ifdef DUPKEY
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
#else
void printKeys(Node *node, bool compressed) {
    for (uint32_t i = 0; i < node->size; i++) {
        if (compressed) {
            cout << GetKey(node, i) << ",";
        }
        else {
            cout << node->prefix->addr() << GetKey(node, i) << ",";
        }
    }
}
#endif

void printKeys_db2(DB2Node *node, bool compressed) {
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
                    cout << node->prefixMetadata.at(i).prefix + node->keys.at(l).value
                         << ",";
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