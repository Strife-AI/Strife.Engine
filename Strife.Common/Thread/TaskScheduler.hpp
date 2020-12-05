#pragma once
#include <functional>
#include <memory>
#include <unordered_set>


#include "ThreadPool.hpp"

enum class ScheduledTaskRunState
{
    Scheduled,
    Started,
    Done
};

struct ScheduledTaskSettings
{
    std::optional<int> runCount;
    std::optional<float> runsPerSecond;

};

struct ScheduledTask
{
    float runTime;
    std::shared_ptr<IThreadPoolWorkItem> workItem;
};

class TaskScheduler
{
public:
    void Start(std::shared_ptr<ScheduledTask> task)
    {
        _taskListLock.Lock();
        _tasks.insert(task);
        _taskListLock.Unlock();
    }

    void Run();

    static TaskScheduler* GetInstance() { return nullptr; }

private:
    SpinLock _taskListLock;
    std::unordered_set<std::shared_ptr<ScheduledTask>> _tasks;
    std::vector<std::shared_ptr<ScheduledTask>> _completeTasks;
};