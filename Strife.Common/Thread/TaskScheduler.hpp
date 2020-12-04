#pragma once
#include <functional>
#include <memory>
#include <unordered_set>


#include "ThreadPool.hpp"

enum class ScheduledTaskRunState
{
    Running,
    Stopped
};

struct ScheduledTaskSettings
{
    std::optional<int> runCount;
    std::optional<float> runsPerSecond;
    std::function<void()> runCallback;
};

struct ScheduledTask
{
    void UpdateSettings(ScheduledTaskSettings& newSettings)
    {
        _changeSettingsLock.Lock();
        _newSettings = std::move(newSettings);
        _changeSettingsLock.Unlock();
    }

private:
    friend class TaskScheduler;

    void ApplyNewSettings()
    {
        _changeSettingsLock.Lock();
        if(_newSettings.has_value())
        {
            _currentSettings = std::move(*_newSettings);
            _newSettings = std::nullopt;
        }
        _changeSettingsLock.Unlock();
    }

    std::optional<ScheduledTaskSettings> _newSettings;
    SpinLock _changeSettingsLock;
    ScheduledTaskSettings _currentSettings;

    std::shared_ptr<IThreadPoolWorkItem> _lastWorkItem;
    float _nextRunTime = 0;
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

    void Stop(std::shared_ptr<ScheduledTask> task)
    {
        _taskListLock.Lock();
        _tasks.erase(task);
        _taskListLock.Unlock();
    }

    void Run();

    static TaskScheduler* GetInstance() { return nullptr; }

private:
    SpinLock _taskListLock;
    std::unordered_set<std::shared_ptr<ScheduledTask>> _tasks;
};