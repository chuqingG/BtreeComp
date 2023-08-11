#pragma once
#include "node_mt_mc.h"


// Constructor of Node
Node::Node(){
    size = 0;
    prefix = "";
    id = rand();
    next = nullptr;
    lowkey = "";
    highkey = MAXHIGHKEY;
    level = 0;
}

//Destructor of Node
Node::~Node(){
    for (Node *childptr : ptrs) {
        delete childptr;
    }
}

void Node::lock(accessMode mode) {
    switch (mode)
    {
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
    switch (mode)
    {
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

DB2Node::DB2Node(){
    size = 0;
    id = rand();
    next = nullptr;
    highkey = MAXHIGHKEY;
    level = 0;
}

//Destructor of Node
DB2Node::~DB2Node(){
    for (DB2Node *childptr : ptrs) {
        delete childptr;
    }
}

void DB2Node::lock(accessMode mode) {
    switch (mode)
    {
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

void DB2Node::unlock(accessMode mode) {
    switch (mode)
    {
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

keyMyISAM::keyMyISAM(string v, int p){
    value = v;
    setPrefix(p);
}

int keyMyISAM::getPrefix(){
    return is1Byte ? prefix[0] : (prefix[1] << 8) | prefix[0];
}

void keyMyISAM::setPrefix(int p){
    if (p <= 127){
        prefix = new u_char[1];
        prefix[0] = (u_char)(p);
        is1Byte = true;
    }
    // Prefix is 2 bytes
    else{
        prefix = new u_char[2];
        prefix[0] = (u_char)(p);
        prefix[1] = (u_char)((p >> 8));
        is1Byte = false;
    }
}

int keyMyISAM::getSize(){
    int prefixBytes = getPrefix() <= 127 ? 1 : 2;
    return value.length() + prefixBytes;
}

NodeMyISAM::NodeMyISAM(){
    size = 0;
    id = rand();
    next = nullptr;
    highkey = MAXHIGHKEY;
    level = 0;
}

//Destructor of Node
NodeMyISAM::~NodeMyISAM(){
    for (NodeMyISAM *childptr : ptrs) {
        delete childptr;
    }
}

void NodeMyISAM::lock(accessMode mode) {
    switch (mode)
    {
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
    switch (mode)
    {
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

KeyWT::KeyWT(string val, uint8_t p){
  value = val;
  prefix = p;
  isinitialized = false;
  initialized_value = "";
  mLock = new std::shared_mutex();
}

// Get size of key
int KeyWT::getSize(){
  return sizeof(prefix) + value.length();
}

NodeWT::NodeWT(){
  size = 0;
  id = rand();
  next = nullptr; 
  highkey = MAXHIGHKEY;
  level = 0;
  prefixstart = 0;
  prefixstop = 0;
}

//Destructor of Node
NodeWT::~NodeWT(){
    for (NodeWT *childptr : ptrs) {
        delete childptr;
    }
}

void NodeWT::lock(accessMode mode) {
    switch (mode)
    {
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
    switch (mode)
    {
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
KeyPkB::KeyPkB(int pos, string value, char *ptr){
  offset = pos;
  if (pos < value.length()){
    string substr = value.substr(pos, PKB_LEN);
    strcpy(partialKey, substr.c_str());
    pkLength = substr.length();
  }else{
    memset(partialKey, 0, sizeof(partialKey));
    pkLength = 0;
  }
  original = ptr;
}

// Constructor of Key when pKB is disabled
KeyPkB::KeyPkB(string value){
  offset = 0;
  pkLength = 0;
  memset(partialKey, 0, sizeof(partialKey));
  original = new char[value.length() + 1];
  strcpy(original, value.c_str());
}

int KeyPkB::getSize(){
  return sizeof(offset) + PKB_LEN + sizeof(original);
}

void KeyPkB::updateOffset(int newOffset){
  offset = newOffset;
  string str = original;
  if (newOffset < str.length()){
    string substr = str.substr(newOffset, PKB_LEN);
    strcpy(partialKey, substr.c_str());
    pkLength = substr.length();
  }else{
    memset(partialKey, 0, sizeof(partialKey));
    pkLength = 0;
  }
}

NodePkB::NodePkB(){
  size = 0;
  id = rand();
  next = nullptr;
  lowkey = ""; 
  highkey = MAXHIGHKEY;
  level = 0;
}

//Destructor of Node
NodePkB::~NodePkB(){
    for (NodePkB *childptr : ptrs) {
        delete childptr;
    }
}

void NodePkB::lock(accessMode mode) {
    switch (mode)
    {
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
    switch (mode)
    {
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

void printKeys(Node *node, bool compressed){
    string prev_key = "";
    string curr_key = "";
    for (uint32_t i = 0; i < node->keys.size(); i++){
        if (compressed){
            cout << node->keys.at(i) << ",";
        }else{
            cout << node->prefix + node->keys.at(i) << ",";
        }
    }
}

void printKeys_db2(DB2Node *node, bool compressed){
    for (uint32_t i = 0; i < node->prefixMetadata.size(); i++){
        if (compressed){
            cout << "Prefix " << node->prefixMetadata.at(i).prefix << ": ";
        }
        for (int l = node->prefixMetadata.at(i).low;
                l <= node->prefixMetadata.at(i).high; l++){
            if (compressed){
                cout << node->keys.at(l) << ",";
            }else{
                cout << node->prefixMetadata.at(i).prefix + node->keys.at(l) << ",";
            }
        }
    }
}

void printKeys_myisam(NodeMyISAM *node, bool compressed){
    string prev_key = "";
    string curr_key = "";
    for (uint32_t i = 0; i < node->keys.size(); i++){
        if (compressed || node->keys.at(i).getPrefix() == 0){
            curr_key = node->keys.at(i).value;
        }else{
            curr_key = prev_key.substr(0, node->keys.at(i).getPrefix()) + node->keys.at(i).value;
        }
        cout << curr_key << ",";
        prev_key = curr_key;
    }
}

void printKeys_wt(NodeWT *node, bool compressed){

  string prev_key = "";
  string curr_key = "";
  for (int i = 0; i < node->keys.size(); i++){
    if (compressed || node->keys.at(i).prefix == 0){
      curr_key = node->keys.at(i).value;
    } else {
      curr_key = prev_key.substr(0, node->keys.at(i).prefix) + node->keys.at(i).value;
    }
    cout << curr_key << ",";
    prev_key = curr_key;
  }
}

void printKeys_pkb(NodePkB *node, bool compressed){
  string curr_key;
  for (int i = 0; i < node->keys.size(); i++){
    if (compressed){
      curr_key = node->keys.at(i).partialKey;
    }else{
      curr_key = node->keys.at(i).original;
    }
    cout << curr_key << ",";
  }
}