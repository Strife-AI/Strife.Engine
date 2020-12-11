#pragma once

#include <queue>
#include "Thread/SpinLock.hpp"

template<typename T>
class ConcurrentQueue
{
public:
    void Enqueue(const T& item);

	template<class... Args>
    void Emplace(Args&&... args);

    void Dequeue();
    const T& Front();
    bool TryDequeue(T& outItem);

    int Size() const { return _queue.size(); }
	bool IsEmpty() const { return Size() == 0; }

private:
	SpinLock _lock;
	std::queue<T> _queue;
};

template <typename T>
void ConcurrentQueue<T>::Enqueue(const T& item)
{
    _lock.Lock();
    {
        _queue.push(item);
    }
    _lock.Unlock();
}

template <typename T>
template <class ... Args>
void ConcurrentQueue<T>::Emplace(Args&&... args)
{
    _lock.Lock();
    {
        _queue.emplace(std::forward<Args>(args)...);
    }
    _lock.Unlock();
}

template <typename T>
void ConcurrentQueue<T>::Dequeue()
{
    Assert(_queue.size() > 0);

    _lock.Lock();
    _queue.pop();
    _lock.Unlock();
}

template <typename T>
const T& ConcurrentQueue<T>::Front()
{
    _lock.Lock();
    auto& item = _queue.front();
    _lock.Unlock();

    return item;
}

template <typename T>
bool ConcurrentQueue<T>::TryDequeue(T& outItem)
{
    bool hasItem = false;

    _lock.Lock();
    {
        if (!_queue.empty())
        {
            outItem = _queue.front();
            _queue.pop();
            hasItem = true;
        }
    }
    _lock.Unlock();

    return hasItem;
}
