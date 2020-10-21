#include "Timer.hpp"

void TimerManager::StartTimer(float timeSeconds, const std::function<void()>& callback)
{
    _timers.emplace_back(timeSeconds, callback);
}

void TimerManager::StartEntityTimer(float timeSeconds, const std::function<void()>& callback, Entity* entity)
{
    _entityTimerInstances.emplace_back(timeSeconds, callback, entity);
}

void TimerManager::TickTimers(float timeSeconds)
{
    {
        auto it = _timers.begin();
        while (it != _timers.end())
        {
            auto timer = it++;
            timer->timeLeft -= timeSeconds;
            if (timer->timeLeft <= 0)
            {
                timer->callback();
                _timers.erase(timer);
            }
        }
    }
    {
        auto it = _entityTimerInstances.begin();
        while (it != _entityTimerInstances.end())
        {
            auto timer = it++;
            timer->timeLeft -= timeSeconds;

            if (timer->timeLeft <= 0)
            {
                if (timer->entity.IsValid())
                {
                    timer->callback();
                }
                
                _entityTimerInstances.erase(timer);
            }
        }
    }
}
