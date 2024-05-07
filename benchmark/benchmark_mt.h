#pragma once
#include <vector>
#include <sstream>
#include <iomanip>
#include <string>
#include <cmath>
#include "../utils/config.h"
#include "../utils/util.h"

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
    virtual TreeStatistics CalcStatistics() = 0;

    int threadPoolSize = 16;
    int column_num = 1;
    void set_thread(int n) {
        threadPoolSize = n;
    }
};

// Changing key word to char...
class Benchmark_c {
public:
    virtual ~Benchmark_c() = default;
    virtual void InitializeStructure(int thread_num, int col = 1) = 0;
    virtual void DeleteStructure() = 0;
    virtual void Insert(const std::vector<char *> &numbers) = 0;
    virtual bool Search(const std::vector<char *> &numbers) = 0;
    virtual TreeStatistics CalcStatistics() = 0;
    int threadPoolSize = 16;
    int column_num = 1;
    void set_thread(int n) {
        threadPoolSize = n;
    }
};

class BPTreeStdBenchmark : public Benchmark_c {
public:
    ~BPTreeStdBenchmark() override {
        delete _tree;
    }

    void InitializeStructure(int thread_num, int col = 1) override {
        _tree = new BPTreeMT();
        this->set_thread(thread_num);
    }

    void DeleteStructure() override {
        delete _tree;
        _tree = nullptr;
    }

    void Insert(const vector<char *> &values) override {
        // atomic<int> non_successful_searches(0);
        std::vector<std::thread> threads;
        int totalKeys = values.size();
        int keysPerThread = totalKeys / threadPoolSize;

        for (int i = 0; i < threadPoolSize; ++i) {
            int start = i * keysPerThread;
            int end = (i + 1) * keysPerThread;
            threads.emplace_back(&BPTreeStdBenchmark::insertWork, this, values, start, end, i /*, &non_successful_searches */);
        }

        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();

        }
        // vector<bool> flag(values.size());
        // _tree->printTree(_tree->getRoot(), flag, true);
    }

    bool Search(const std::vector<char *> &values) override {
        // create a thread pool with threadPoolSize threads

        // atomic<int> non_successful_searches(0);
          std::vector<std::thread> threads;
        int totalKeys = values.size();
        int keysPerThread = totalKeys / threadPoolSize;

        for (int i = 0; i < threadPoolSize; ++i) {
            int start = i * keysPerThread;
            int end = (i + 1) * keysPerThread;
            threads.emplace_back(&BPTreeStdBenchmark::searchWork, this, values, start, end, i /*, &non_successful_searches */);
        }

        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread.join();

        }
        //  cout << "Count: " << non_successful_searches << endl;
        return true;
        // return non_successful_searches == 0;

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

    TreeStatistics CalcStatistics() override {
        TreeStatistics statistics;
        statistics.height = _tree->getHeight(_tree->getRoot());
        _tree->getSize(_tree->getRoot(), statistics.numNodes, statistics.nonLeafNodes, statistics.numKeys, statistics.totalBranching, statistics.totalKeySize, statistics.totalPrefixSize);
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
    void searchWork(const std::vector<char *> &values, int start, int end, int key/*,  atomic<int> *failure*/) {
        for (uint32_t i = start; i < end; i++) {
            if (_tree->search(values.at(i)) == -1) {
                // cout << i << "\n";
                // cout << "Failed for " << values.at(i) << endl;
                //*failure += 1;
            }
        };
        // cout << key << " from " << start << " to " << end << " exited\n";
    }

    void insertWork(const std::vector<char *> &values, int start, int end, int key/*,  atomic<int> *failure*/) {
        for (uint32_t i = start; i < end; i++) {
            _tree->insert(values.at(i));
        };
        // cout << key << " from " << start << " to " << end << " exited\n";
    }
    
protected:
    BPTreeMT *_tree;
};

class BPTreeHeadCompBenchmark : public BPTreeStdBenchmark {
public:
    void InitializeStructure(int thread_num, int col = 1) override {
        _tree = new BPTreeMT(true, false);
        this->set_thread(thread_num);
    }
};

class BPTreeTailCompBenchmark : public BPTreeStdBenchmark {
public:
    void InitializeStructure(int thread_num, int col = 1) override {
        _tree = new BPTreeMT(false, true);
        this->set_thread(thread_num);
    }
};

class BPTreeHeadTailCompBenchmark : public BPTreeStdBenchmark {
public:
    void InitializeStructure(int thread_num, int col = 1) override {
        _tree = new BPTreeMT(true, true);
        this->set_thread(thread_num);
    }
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
