#pragma once
#include <stdio.h>
#include <iostream>
#include "node_disk.h"

class DskManager {
public:
    FILE *fp;
    bool isopen;
    int page_count;
    DskManager(const char *filename) {
        fp = fopen(filename, "w+");
        fclose(fp);
        fp = fopen(filename, "r+");
        isopen = true;
    }
    ~DskManager() {
        fclose(fp);
    }

    Node *get_new_leaf() {
        Node *n = new Node();
        n->id = page_count++;
        return n;
    }
};