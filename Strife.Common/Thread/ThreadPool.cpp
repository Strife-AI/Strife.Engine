#include "ThreadPool.hpp"

std::unique_ptr<ThreadPool> ThreadPool::_instance;

ThreadPool::ThreadPool(int maxThreads)
{
    for(int i = 0; i < maxThreads; ++i)
    {
        _threads.emplace_back(&ThreadPool::WorkerThread, this);
    }
}

ThreadPool::~ThreadPool()
{
    Stop();
}

void ThreadPool::StartItem(const std::shared_ptr<IThreadPoolWorkItem>& item)
{
    _workQueue.Enqueue(item);
}

ThreadPool* ThreadPool::GetInstance()
{
    if(_instance == nullptr)
    {
        _instance = std::make_unique<ThreadPool>(4);
    }

    return _instance.get();
}

void ThreadPool::Stop()
{
    _stopThreads = true;

    for(auto& thread : _threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
}

void ThreadPool::WorkerThread()
{
    return;
    while (!_stopThreads)
    {
        std::shared_ptr<IThreadPoolWorkItem> item;
        if(_workQueue.TryDequeue(item))
        {
            item->Run();
        }
        else
        {
            // TODO pause for a millisecond to prevent high CPU usage?
        }
    }
}
