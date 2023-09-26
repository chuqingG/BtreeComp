#pragma once
#include <stdio.h>
#include <iostream>
#include "node_disk.h"

class DskManager {
public:
    FILE *fp;
    bool isopen;
    uint32_t page_count;
    char *path;
    DskManager(const char *filename) {
        page_count = 0;
        // create the file
        fp = fopen(filename, "w+");
        // fclose(fp);
        path = new char[strlen(filename) + 1];
        strcpy(path, filename);
    }
    ~DskManager() {
        cout << "total leaf: " << unsigned(page_count) << endl;
        fclose(fp);
        delete[] path;
    }

    void close() {
        fclose(fp);
    }

    void write() {
        fp = fopen(path, "a+");
        rewind(fp);
    }

    void read() {
        fopen(path, "r");
    }

    Node *get_new_leaf() {
        Node *n = new Node();
        n->id = page_count++;
        return n;
    }
};