#pragma once

#include <functional>
#include <list>

#include "Entity.hpp"

class TimerManager
{
public:
    void StartTimer(float timeSeconds, const std::function<void()>& callback);
    void StartEntityTimer(float timeSeconds, const std::function<void()>& callback, Entity* entity);
    void TickTimers(float timeSeconds);

private:
    struct TimerInstance
    {
        TimerInstance(float timeLeft_, const std::function<void()>& callback_)
            : timeLeft(timeLeft_),
            callback(callback_)
        {
            
        }

        float timeLeft;
        std::function<void()> callback;
    };

    struct EntityTimerInstance
    {
        EntityTimerInstance(float timeLeft_, const std::function<void()>& callback_, Entity* entity)
            : timeLeft(timeLeft_),
            callback(callback_),
            entity(entity)
        {

        }

        float timeLeft;
        std::function<void()> callback;
        EntityReference<Entity> entity;
    };

    std::list<TimerInstance> _timers;
    std::list<EntityTimerInstance> _entityTimerInstances;
};
