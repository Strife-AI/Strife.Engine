#include "TaskScheduler.hpp"

std::unique_ptr<TaskScheduler> TaskScheduler::_instance;

static void SleepSeconds(float microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds((int)(microseconds / 1000000)));
}

static float GetTimeSeconds()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    auto deltaTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
    return static_cast<float>(deltaTimeMicroseconds.count()) / 1000000;
}

TaskScheduler::TaskScheduler()
    : _workThread(&TaskScheduler::Run, this)
{

}

TaskScheduler::~TaskScheduler()
{
    _isDone = true;
    if(_workThread.joinable())
    {
        _workThread.join();
    }
}

void TaskScheduler::Run()
{
    return;
    while (!_isDone)
    {
        ThreadPool* threadPool = ThreadPool::GetInstance();

        // TODO: some sort of sleeping if no tasks are ready?

        _taskListLock.Lock();
        {
            for (auto& task : _tasks)
            {
                float now = GetTimeSeconds();
                if (now >= task->runTime)
                {
                    task->startTime = now;
                    threadPool->StartItem(task->workItem);
                    _completeTasks.push_back(task);
                }
            }

            for (auto& completeTask : _completeTasks)
            {
                _tasks.erase(completeTask);
            }
            _completeTasks.clear();
        }
        _taskListLock.Unlock();
    }
}

TaskScheduler* TaskScheduler::GetInstance()
{
    // TODO: does this need to be thread safe?
    if(_instance == nullptr)
    {
        _instance = std::make_unique<TaskScheduler>();
    }

    return _instance.get();
}
