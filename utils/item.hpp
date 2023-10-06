#pragma once
#include <iostream>
#include <cstring>

struct Item {
    char *addr;
    uint16_t size;
    bool newallocated = false;
    Item() {
        // this works for wt, myisam, pkb and splitkey,
        // temporarily fetch a key
        newallocated = false;
    }
    Item(bool allocated) {
        // this works for std compression: prefix, l/hkey
        addr = new char[1];
        addr[0] = '\0';
        size = 0;
        newallocated = true;
    }
    Item(Item &old) {
        if (old.size) {
            addr = new char[old.size + 1];
            strcpy(addr, old.addr);
            size = old.size;
            newallocated = true;
        }
        else {
            newallocated = false;
            size = 0;
        }
    }
    Item(char *p, uint8_t l, bool allo) {
        addr = p;
        size = l;
        newallocated = allo;
    }
    ~Item() {
        if (newallocated) {
            delete[] addr;
        }
    }
    Item &operator=(Item &old) {
        addr = old.addr;
        size = old.size;
        newallocated = false;
        return *this;
    }
};