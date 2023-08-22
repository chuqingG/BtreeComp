#pragma once

#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include "../include/config.h"
#include "../include/util.h"

#include "../tree_mt/btree_std_mt.cpp"

#ifndef CHARALL
#include "../tree_mt/btree_db2_mt.cpp"
#include "../tree_mt/btree_myisam_mt.cpp"
#include "../tree_mt/btree_wt_mt.cpp"
#include "../tree_mt/btree_pkb_mt.cpp"
#endif

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <boost/atomic.hpp>

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

class Benchmark {
public:
    virtual ~Benchmark() = default;
    virtual void InitializeStructure(int thread_num, int col = 1) = 0;
    virtual void DeleteStructure() = 0;
    virtual void Insert(const std::vector<char *> &numbers) = 0;
    virtual bool Search(const std::vector<char *> &numbers) = 0;
#ifndef CHARALL
    virtual bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) = 0;
    virtual bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) = 0;
#endif
    virtual TreeStatistics CalcStatistics() = 0;

    int threadPoolSize = 16;
    int column_num = 1;
    void set_thread(int n) {
        threadPoolSize = n;
    }
};

class BPTreeStdBenchmark : public Benchmark {
public:
    ~BPTreeStdBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
#ifdef MULTICOL_COMPRESSION
        tree_ = new BPTreeMT(col);
#else
        tree_ = new BPTreeMT();
#endif
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<char *> &values) override {
#ifndef MYDEBUG
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
        // vector<bool> flag(values.size());
        // tree_->printTree(tree_->getRoot(), flag, true);
#else
        for (uint32_t i = 0; i < values.size(); ++i)
            tree_->insert(values.at(i));
            // vector<bool> flag(values.size());
            // tree_->printTree(tree_->getRoot(), flag, true);
#endif
    }

    bool Search(const std::vector<char *> &values) override {
#ifndef MYDEBUG
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
#else
        int non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            if (tree_->search(values.at(i)) == -1) {
                cout << "Failed for " << values.at(i) << endl;
                non_successful_searches += 1;
            }
        }
        return non_successful_searches == 0;
#endif
#ifdef MYDEBUG
        // boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < values.size(); ++i) {
            auto t1 = std::chrono::system_clock::now();
            if (tree_->search(values.at(i)) == -1) {
                cout << "Failed for " << values.at(i) << endl;
                non_successful_searches += 1;
            }
            atomicTotalTime += static_cast<double>(std::chrono::duration_cast<
                                                       std::chrono::nanoseconds>(std::chrono::system_clock::now() - t1)
                                                       .count())
                               / 1e9;
        }
        // pool.wait();
        timeSpent = atomicTotalTime;
        return non_successful_searches == 0;
#endif
    }

#ifndef CHARALL
    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }
#endif

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeMT *tree_;
};

class BPTreeHeadCompBenchmark : public Benchmark {
public:
    ~BPTreeHeadCompBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
#ifdef MULTICOL_COMPRESSION
        tree_ = new BPTreeMT(col, true, false);
#else
        tree_ = new BPTreeMT(true, false);
#endif
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<char *> &values) override {
#ifndef MYDEBUG
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
        // std::cout << "=========head==============" << std::endl;
        // vector<bool> flag(values.size());
        // tree_->printTree(tree_->getRoot(), flag);
#else
        for (uint32_t i = 0; i < values.size(); ++i) {
            tree_->insert(values.at(i));
            // vector<bool> flag(i+1);
            // tree_->printTree(tree_->getRoot(), flag, true);
        }
        // vector<bool> flag(values.size());
        std::cout << "=====TREE====" << std::endl;
        // tree_->printTree(tree_->getRoot(), flag);
#endif
    }

    bool Search(const std::vector<char *> &values) override {
#ifndef MYDEBUG
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
#else
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            if (tree_->search(values.at(i)) == -1) {
                // cout << "Failed for " << values.at(i) << endl;
                non_successful_searches += 1;
            }
        }
        std::cout << "Errors count: " << non_successful_searches << std::endl;
#endif
        return non_successful_searches == 0;
    }

#ifndef CHARALL
    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }
#endif

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeMT *tree_;
};

class BPTreeTailCompBenchmark : public Benchmark {
public:
    ~BPTreeTailCompBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
#ifdef MULTICOL_COMPRESSION
        tree_ = new BPTreeMT(col, false, true);
#else
        tree_ = new BPTreeMT(false, true);
#endif
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<char *> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
        // std::cout << "=========tail==============" << std::endl;
        // vector<bool> flag(values.size());
        // tree_->printTree(tree_->getRoot(), flag, true);
    }

    bool Search(const std::vector<char *> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
    }

#ifndef CHARALL
    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }

#endif

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeMT *tree_;
};

class BPTreeHeadTailCompBenchmark : public Benchmark {
public:
    ~BPTreeHeadTailCompBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
#ifdef MULTICOL_COMPRESSION
        tree_ = new BPTreeMT(col, true, true);
#else
        tree_ = new BPTreeMT(true, true);
#endif
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<char *> &values) override {
#ifndef MYDEBUG
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
#else
        for (uint32_t i = 0; i < values.size(); ++i)
            tree_->insert(values.at(i));
        vector<bool> flag(values.size());
        tree_->printTree(tree_->getRoot(), flag, true);
#endif
    }

    bool Search(const std::vector<char *> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
    }

#ifndef CHARALL

    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }

#endif

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeMT *tree_;
};

#ifndef CHARALL

class BPTreeDB2Benchmark : public Benchmark {
public:
    ~BPTreeDB2Benchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
        tree_ = new BPTreeDB2MT();
        column_num = col;
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
        // std::cout << "=========DB2==============" << std::endl;
        // vector<bool> flag(values.size());
        // tree_->printTree(tree_->getRoot(), flag, true);
    }

    bool Search(const std::vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
    }

    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeDB2MT *tree_;
};

class BPTreeMyISAMBenchmark : public Benchmark {
public:
    ~BPTreeMyISAMBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
        tree_ = new BPTreeMyISAMMT();
        column_num = col;
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
        // std::cout << "=========myisam==============" << std::endl;
        // vector<bool> flag(values.size());
        // tree_->printTree(tree_->getRoot(), flag, true);
    }

    bool Search(const std::vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
    }

    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeMyISAMMT *tree_;
};

class BPTreeWTBenchmark : public Benchmark {
public:
    ~BPTreeWTBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
        tree_ = new BPTreeWTMT(true);
        column_num = col;
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
        // std::cout << "=========wiretiger==============" << std::endl;
        // vector<bool> flag(values.size());
        // tree_->printTree(tree_->getRoot(), flag, true);
    }

    bool Search(const std::vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
    }

    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreeWTMT *tree_;
};

class BPTreePkBBenchmark : public Benchmark {
public:
    ~BPTreePkBBenchmark() override {
        delete tree_;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
        tree_ = new BPTreePkBMT();
        column_num = col;
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete tree_;
        tree_ = nullptr;
    }

    void Insert(const vector<string> &values) override {
        for (uint32_t i = 0; i < values.size(); i++) {
            char *ptr = new char[values.at(i).length() + 1];
            strcpy(ptr, values.at(i).c_str());
            tree_->strPtrMap[ptr] = ptr;
        }

        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] { tree_->insert(values.at(i)); });
        }
        // Wait for all tasks to be completed
        pool.wait();
    }

    bool Search(const std::vector<string> &values) override {
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_searches(0);
        for (uint32_t i = 0; i < values.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                if (tree_->search(values.at(i)) == -1) {
                    cout << "Failed for " << values.at(i) << endl;
                    non_successful_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_searches == 0;
    }

    // Values must be sorted
    bool SearchRange(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_range_searches(0);
        boost::atomic<double> atomicTotalTime;
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->searchRange(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_range_searches += 1;
                }
            });
        }
        pool.wait();
        return non_successful_range_searches == 0;
    }

    bool BackwardScan(const std::map<string, int> valuesFreq, const std::vector<int> minIdxs) override {
        int range_size = valuesFreq.size() / 3;
        vector<string> keys = get_map_keys(valuesFreq);
        // create a thread pool with threadPoolSize threads
        boost::asio::thread_pool pool(threadPoolSize);
        atomic<int> non_successful_backward_scans(0);
        for (uint32_t i = 0; i < minIdxs.size(); ++i) {
            boost::asio::post(pool, [&, i] {
                string min = keys.at(minIdxs[i]);
                string max = keys.at(minIdxs[i] + range_size);
                int entries = tree_->backwardScan(min, max);
                int expected = get_map_values_in_range(valuesFreq, min, max);
                if (entries != expected) {
                    cout << "Failure number of entries " << entries << " , expected " << expected << endl;
                    non_successful_backward_scans += 1;
                }
            });
        }
        pool.wait();
        return non_successful_backward_scans == 0;
    }

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = tree_->getHeight(tree_->getRoot());
        tree_->getSize(tree_->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
        cout << "Height of tree " << statistics.height << endl;
        cout << "Num nodes " << statistics.numNodes << endl;
        cout << "Num non-leaf nodes " << statistics.nonLeafNodes << endl;
        cout << "Num keys " << statistics.numKeys << endl;
        cout << "Avg key size based on keys " << statistics.totalKeySize / (double)statistics.numKeys << endl;
        cout << "Avg prefix size " << statistics.totalPrefixSize / (double)statistics.numKeys << endl;
        cout << "Avg branching size " << statistics.totalBranching / (double)statistics.nonLeafNodes << endl;
        return statistics;
    }

private:
    BPTreePkBMT *tree_;
};

#endif