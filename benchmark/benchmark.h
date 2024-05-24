#pragma once

#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include "../utils/util.h"
#include "../tree/btree_std.cpp"
#include "../tree/btree_db2.cpp"
#include "../tree/btree_myisam.cpp"
#include "../tree/btree_wt.cpp"
#include "../tree/btree_pkb.cpp"
// #include "art.hpp"
// using namespace std;

struct TreeStatistics {
    double avgNodeSize = 0;
    int numNodes = 0;
    unsigned long totalKeySize = 0;
    int totalPrefixSize = 0;
    int numKeys = 0;
    int nonLeafNodes = 0;
    int totalBranching = 0;
    int height = 0;
};

// Changing key word to char...
class Benchmark {
public:
    virtual ~Benchmark() = default;
    virtual void InitializeStructure() = 0;
    virtual void DeleteStructure() = 0;
    virtual void Insert(const std::vector<char *> &numbers) = 0;
    virtual bool Search(const std::vector<char *> &numbers) = 0;
    virtual bool SearchRange(std::vector<char *> &sorted_values,
                             std::vector<int> &minIdxs) = 0;
    virtual TreeStatistics CalcStatistics() = 0;
};

class BPTreeStdBenchmark : public Benchmark {
public:
    ~BPTreeStdBenchmark() override {
        delete _tree;
    }

    virtual void InitializeStructure() override {
        _tree = new BPTree();
    }

    void DeleteStructure() override {
        delete _tree;
        _tree = nullptr;
    }

    void Insert(const vector<char *> &values) override {
        for (uint32_t i = 0; i < values.size(); i++) {
            _tree->insert(values[i]);
                #ifdef PRINT
                cout << "   after " << values[i] << "\n";
                vector<bool> flag(values.size() * 1.25);
                _tree->printTree(_tree->getRoot(), flag, true);
                #endif
        }
        #ifdef PRINT
        vector<bool> flag(values.size() * 1.25);
        _tree->printTree(_tree->getRoot(), flag, true);
        #endif
    }

    bool Search(const std::vector<char *> &values) override {
        int count = 0;
        for (uint32_t i = 0; i < values.size(); i++)
            if (_tree->search(values.at(i)) == -1) {
                #ifdef CHECK
            //             if (i == 2204) {
            //         vector<bool> flag(values.size());
            //         _tree->printTree(_tree->getRoot(), flag, true);
            // }
                if (count < 10)
                cout << "Cannot find " << values[i] << "; " << i << "th value" <<endl;
                #endif
                // return false;
                count++;
            }
        cout << "count: " << count << endl;
#ifdef TRACK_DISTANCE
        cout << "move times: " << _tree->cmp_count << endl
             << "total distance: " << _tree->cmp_length << endl
             << "ratio: " << _tree->cmp_length * 1.0 / _tree->cmp_count << endl;
#endif
        return true;
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRange(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = _tree->getHeight(_tree->getRoot());
        _tree->getSize(_tree->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        return statistics;
    }

protected:
    BPTree *_tree;
};

class BPTreeHeadCompBenchmark : public BPTreeStdBenchmark {
public:
    void InitializeStructure() override {
        _tree = new BPTree(true, false);
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRangeHead(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }
};

class BPTreeTailCompBenchmark : public BPTreeStdBenchmark {
public:
    void InitializeStructure() override {
        _tree = new BPTree(false, true);
    }
};

class BPTreeHeadTailCompBenchmark : public BPTreeStdBenchmark {
public:
    void InitializeStructure() override {
        _tree = new BPTree(true, true);
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRangeHead(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }
};

class BPTreeDB2Benchmark : public Benchmark {
public:
    ~BPTreeDB2Benchmark() override {
        delete _tree;
    }

    void InitializeStructure() override {
        _tree = new BPTreeDB2();
    }

    void DeleteStructure() override {
        delete _tree;
        _tree = nullptr;
    }

    void Insert(const std::vector<char *> &values) override {
        for (uint32_t i = 0; i < values.size(); ++i) {
            _tree->insert(values.at(i));
            // vector<bool> flag(i + 1);
            // _tree->printTree(_tree->getRoot(), flag, true);
        }
        vector<bool> flag(values.size());
        _tree->printTree(_tree->getRoot(), flag, true);
    }

    // bool Search(const std::vector<char *> &values) override {
    //     for (uint32_t i = 0; i < values.size(); ++i)
    //         if (_tree->search(values.at(i)) == -1)
    //             return false;
    //     return true;
    // }
    bool Search(const std::vector<char *> &values) override {
        int count = 0;
        for (uint32_t i = 0; i < values.size(); ++i)
            if (_tree->search(values.at(i)) == -1) {

                count++;
            }
        cout << "count:" << count << endl;
        return true;
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRange(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = _tree->getHeight(_tree->getRoot());
        _tree->getSize(_tree->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        return statistics;
    }

private:
    BPTreeDB2 *_tree;
};

class BPTreeMyISAMBenchmark : public Benchmark {
public:
    ~BPTreeMyISAMBenchmark() override {
        delete _tree;
    }

    void InitializeStructure() override {
        _tree = new BPTreeMyISAM(true);
    }

    void DeleteStructure() override {
        delete _tree;
        _tree = nullptr;
    }

    void Insert(const vector<char *> &values) override {
        for (uint32_t i = 0; i < values.size(); ++i) {
            // if (i == values.size() - 1)
            //     cout << "stop here" << endl;
            _tree->insert(values.at(i));
            // vector<bool> flag(i + 1);
            // _tree->printTree(_tree->getRoot(), flag, true);
        }
        // vector<bool> flag(values.size() * 1.25);
        // _tree->printTree(_tree->getRoot(), flag, true);
    }

    bool Search(const std::vector<char *> &values) override {
        for (uint32_t i = 0; i < values.size(); ++i)
            if (_tree->search(values[i]) == -1) {
                cout << "Cannot find " << values[i] << endl;
                return false;
            }
#ifdef TRACK_DISTANCE
        cout << "move times: " << _tree->decomp_count << endl
             << "total distance: " << _tree->decomp_distance << endl
             << "ratio: " << _tree->decomp_distance * 1.0 / _tree->decomp_count << endl;
        cout << "total scan: " << _tree->total_scan << endl
             << "total cmp: " << _tree->total_need_cmp << endl
             << "percentage: " << _tree->total_need_cmp * 1.0 / _tree->total_scan << endl;
#endif
        return true;
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRange(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = _tree->getHeight(_tree->getRoot());
        _tree->getSize(_tree->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        return statistics;
    }

private:
    BPTreeMyISAM *_tree;
};

class BPTreeWTBenchmark : public Benchmark {
public:
    ~BPTreeWTBenchmark() override {
        delete _tree;
    }

    void InitializeStructure() override {
        _tree = new BPTreeWT(true, true);
    }

    void DeleteStructure() override {
        delete _tree;
        _tree = nullptr;
    }

    void Insert(const vector<char *> &values) override {
        for (uint32_t i = 0; i < values.size(); ++i) {
            _tree->insert(values[i]);
            // vector<bool> flag(i + 1);
            // _tree->printTree(_tree->getRoot(), flag, true);
            // _tree->printTree(_tree->getRoot(), flag, false);
        }
        // vector<bool> flag(values.size() * 1.25);
        // _tree->printTree(_tree->getRoot(), flag, true);
    }

    bool Search(const std::vector<char *> &values) override {
        int count = 0;
        for (uint32_t i = 0; i < values.size(); ++i)
            if (_tree->search(values[i]) == -1)
                count++;
        // return false;
        cout << "count: " << count << endl;
        return true;
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRange(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = _tree->getHeight(_tree->getRoot());
        _tree->getSize(_tree->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        return statistics;
    }

private:
    BPTreeWT *_tree;
};

class BPTreePkBBenchmark : public Benchmark {
public:
    ~BPTreePkBBenchmark() override {
        delete _tree;
    }

    void InitializeStructure() override {
        _tree = new BPTreePkB();
    }

    void DeleteStructure() override {
        delete _tree;
        _tree = nullptr;
    }

    void Insert(const vector<char *> &values) override {
        for (uint32_t i = 0; i < values.size(); ++i) {
            _tree->insert(values.at(i));
            // vector<bool> flag(i + 1);
            // _tree->printTree(_tree->getRoot(), flag, true);
        }
    }

    // bool Search(const std::vector<char *> &values) override {
    //     for (uint32_t i = 0; i < values.size(); ++i)
    //         if (_tree->search(values.at(i)) == -1)
    //             return false;
    //     return true;
    // }
    bool Search(const std::vector<char *> &values) override {
        int count = 0;
        for (uint32_t i = 0; i < values.size(); ++i)
            if (_tree->search(values.at(i)) == -1)
                count++;
        cout << "count:" << count << endl;
        return true;
    }

    bool SearchRange(std::vector<char *> &sorted_values,
                     std::vector<int> &minIdxs) override {
        int range_size = sorted_values.size() * RANGE_SCOPE;

        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            char *min = sorted_values.at(minIdxs[i]);
            char *max = sorted_values.at(minIdxs[i] + range_size);
            // cout << "search: [" << min << ", " << max << "]" << endl;
            int entries = _tree->searchRange(min, max);
            // int expected = count_range(sorted_values, minIdxs[i], range_size);
            // if (entries != expected) {
            //     cout << "Failure number of entries " << entries << " , expected "
            //          << expected << endl;
            //     return false;
            // }
        }
        return true;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = _tree->getHeight(_tree->getRoot());
        _tree->getSize(_tree->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        return statistics;
    }

private:
    BPTreePkB *_tree;
};

/*
External tester
*/

// class ARTBenchmark : public Benchmark {
// public:
//     ~ARTBenchmark() override {
//     }

//     virtual void InitializeStructure() override {
//     }

//     void DeleteStructure() override {
//     }

//     void Insert(const vector<char *> &keys) override {
//         int v = 1;
//         for (uint32_t i = 0; i < keys.size(); i++) {
//             _tree.set(keys[i], v);
//         }
//     }

//     bool Search(const std::vector<char *> &keys) override {
//         int count = 0;
//         for (uint32_t i = 0; i < keys.size(); i++)
//             if (_tree.get(keys[i]) == -1) {
//                 // cout << "Cannot find " << values[i] << endl;
//                 return false;
//             }
//         return true;
//     }

//     bool SearchRange(std::vector<char *> &sorted_values,
//                      std::vector<int> &minIdxs) override {
//         return true;
//     }

//     TreeStatistics CalcStatistics() override {
//         TreeStatistics statistics;
//         statistics.height = _tree.getHeight();
//         _tree.getSize(statistics.numNodes, statistics.nonLeafNodes,
//                       statistics.totalBranching, statistics.numKeys, // numKeys->usedBranching
//                       statistics.totalKeySize);
//         return statistics;
//     }

// protected:
//     art::art<int> _tree;
// };