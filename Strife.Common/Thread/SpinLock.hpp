#pragma once

#include <atomic>

class SpinLock
{
public:
    bool TryLock();
    void Lock();
    void Unlock();

private:
    std::atomic_flag _lock = ATOMIC_FLAG_INIT;
};

