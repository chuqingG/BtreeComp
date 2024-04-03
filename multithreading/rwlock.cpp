#include "rwlock.h"

using namespace std;

ReadWriteLock::ReadWriteLock()
{
    readers = 0;
    writer = false;
}

void ReadWriteLock::readLock()
{
    unique_lock<mutex> lock(mLock);
    // Wait till there is no writer
    cond_.wait(lock, [this] { return !writer; });
    // while (writer)
    // {
    //     cond.wait(lock);
    // }
    readers++;
}

void ReadWriteLock::readUnlock()
{
    unique_lock<mutex> lock(mLock);
    readers--;
    if (readers == 0)
    {
        cond.notify_one();
    }
}

void ReadWriteLock::writeLock()
{
    unique_lock<mutex> lock(mLock);
    // Wait till there is no reader or writer
    cv_.wait(lock, [this] { return readers_ == 0 && !writer; });
    // while (readers > 0 || writer)
    // {
    //     cond.wait(lock);
    // }
    writer = true;
}

void ReadWriteLock::writeUnlock()
{
    unique_lock<mutex> lock(mLock);
    writer = false;
    cond.notify_all();
}