#pragma once
#include <stdio.h>
#include <iostream>
#include "node_disk.h"

class DskManager {
public:
    FILE *fp;
    bool isopen;
    uint32_t page_count;
    DskManager(const char *filename) {
        page_count = 0;
        fp = fopen(filename, "w+");
        fclose(fp);
        fp = fopen(filename, "r+");
        isopen = true;
    }
    ~DskManager() {
        cout << "total leaf: " << unsigned(page_count) << endl;
        fclose(fp);
    }

    Node *get_new_leaf() {
        Node *n = new Node();
        n->id = page_count++;
        return n;
    }
};