#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

using namespace std;

#ifndef __RWLOCK_H__
#define __RWLOCK_H__

class ReadWriteLock
{
public:
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();
    ReadWriteLock();

private:
    mutex mLock;
    bool writer;
    condition_variable cond;
    int readers;
};

#endif