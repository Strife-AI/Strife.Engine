#include "TaskScheduler.hpp"

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

void TaskScheduler::Run()
{
    _taskListLock.Lock();
    for (auto& task : _tasks)
    {
        task->ApplyNewSettings();

        if(task->_lastWorkItem != nullptr && !task->_lastWorkItem->IsComplete())
        {
            continue;
        }

        bool runBasedOnSchedule = true;
        if (task->_currentSettings.runsPerSecond.has_value())
        {
            float currentTime = GetTimeSeconds();
            runBasedOnSchedule = currentTime >= task->_nextRunTime;

            if(runBasedOnSchedule)
            {
                task->_nextRunTime = currentTime + 1.0f / task->_currentSettings.runsPerSecond.value();
            }
        }

        bool hasAnyRunsLeft = true;
        if(task->_currentSettings.runCount.has_value())
        {
            if(task->_currentSettings.runCount > 0)
            {
                task->_currentSettings.runCount = task->_currentSettings.runCount.value() - 1;
            }
            else
            {
                hasAnyRunsLeft = false;
            }
        }

        if(runBasedOnSchedule && hasAnyRunsLeft)
        {
            
        }
    }
    _taskListLock.Unlock();
}
