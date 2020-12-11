#include "SpinLock.hpp"

bool SpinLock::TryLock()
{
    return !_lock.test_and_set(std::memory_order_acquire);
}

void SpinLock::Lock()
{
    while (_lock.test_and_set(std::memory_order_acquire)) ;

}

void SpinLock::Unlock()
{
    _lock.clear(std::memory_order_release);
}
