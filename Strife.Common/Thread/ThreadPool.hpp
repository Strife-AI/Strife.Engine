#pragma once

#include <memory>
#include <optional>
#include <thread>

#include "Container/ConcurrentQueue.hpp"

struct IThreadPoolWorkItem
{
    virtual ~IThreadPoolWorkItem() = default;

    void Run()
    {
        Execute();
        _isComplete = true;
    }

    bool IsComplete() const
    {
        return _isComplete;
    }

private:
    virtual void Execute() = 0;

    bool _isComplete = false;
};

template<typename TResult>
struct ThreadPoolWorkItem : IThreadPoolWorkItem
{
    bool TryGetResult(TResult& outResult);

protected:
    TResult _result;
};

template <typename TResult>
bool ThreadPoolWorkItem<TResult>::TryGetResult(TResult& outResult)
{
    if (IsComplete())
    {
        outResult = _result;
        return true;
    }
    else
    {
        return false;
    }
}

class ThreadPool
{
public:
    ThreadPool(int maxThreads);
    ~ThreadPool();

    void StartItem(const std::shared_ptr<IThreadPoolWorkItem>& item);

    static ThreadPool* GetInstance();

    void Stop();

private:
    void WorkerThread();

    ConcurrentQueue<std::shared_ptr<IThreadPoolWorkItem>> _workQueue;
    bool _stopThreads = false;
    std::vector<std::thread> _threads;

    static std::unique_ptr<ThreadPool> _instance;
};
