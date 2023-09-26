#pragma once
#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "node_disk.h"

class DskManager {
public:
    FILE *fp;
    bool isopen;
    uint32_t page_count;
    char *path;
    int fd;
    DskManager(const char *filename) {
        page_count = 0;
        // create the file
        fp = fopen(filename, "w+");
        fclose(fp);
        std::filesystem::resize_file(filename, 1024 * 1024 * 1024);

        fd = open(filename, O_RDWR);
        if (fd < 0) {
            printf("open file failed\n");
        }
        // fp = fopen(filename, "w+");
        path = new char[strlen(filename) + 1];
        strcpy(path, filename);
    }
    ~DskManager() {
        cout << "total leaf: " << unsigned(page_count) << endl;
        // fclose(fp);
        delete[] path;
    }

    // void close() {
    //     fclose(fp);
    // }

    // void write() {
    //     fp = fopen(path, "a+");
    //     rewind(fp);
    // }

    // void read() {
    //     fopen(path, "r");
    // }

    Node *get_new_leaf() {
        Node *n = new Node();
        n->id = page_count++;
        return n;
    }

    NodeDB2 *get_new_leaf_db2() {
        NodeDB2 *n = new NodeDB2();
        n->id = page_count++;
        return n;
    }
};