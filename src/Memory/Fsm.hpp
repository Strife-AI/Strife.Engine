#pragma once

#include <cstdint>
#include <functional>


#include "StringId.hpp"
#include "System/BinaryStreamReader.hpp"

template<typename TState, int MaxStates, typename TEvent, int MaxEvents>
class Fsm
{
public:
    Fsm(TState initialState);

    TState State() const { return _currentState; }
    void SetState(TState newState) { _currentState = newState; }
    bool IsValidTransition(TEvent event) const { return GetTransition() != INVALID_TRANSITION; }

    Fsm& TransitionWhen(TState currentState, TEvent when, TState newState);
    TState GetNewState(TEvent event) const;

private:
    enum { INVALID_TRANSITION = INT32_MIN };

    int& GetTransition(TState currentState, TEvent event)
    {
        return _transitions[static_cast<int>(currentState)][static_cast<int>(event)];
    }

    const int& GetTransition(TState currentState, TEvent event) const
    {
        return _transitions[static_cast<int>(currentState)][static_cast<int>(event)];
    }

    TState _currentState;
    int _transitions[MaxStates][MaxEvents];
};

template<typename TState, int MaxStates, typename TEvent, int MaxEvents>
Fsm<TState, MaxStates, TEvent, MaxEvents>::Fsm(TState initialState)
{
    for(int i = 0; i < MaxStates; ++i)
    {
        for(int j = 0; j < MaxEvents; ++j)
        {
            _transitions[i][j] = INVALID_TRANSITION;
        }
    }
}

template<typename TState, int MaxStates, typename TEvent, int MaxEvents>
auto Fsm<TState, MaxStates, TEvent, MaxEvents>::TransitionWhen(TState currentState, TEvent when, TState newState) -> Fsm&
{
    GetTransition(currentState, when) = static_cast<int>(newState);

    return *this;
}

template<typename TState, int MaxStates, typename TEvent, int MaxEvents>
TState Fsm<TState, MaxStates, TEvent, MaxEvents>::GetNewState(TEvent event) const
{
    auto newState = GetTransition(_currentState, event);

    if (newState != INVALID_TRANSITION)
    {
        return static_cast<TState>(newState);
    }
    else
    {
        return _currentState;
    }
}

struct FsmEvent
{
    FsmEvent(StringId type_)
        : type(type_)
    {
        
    }

    StringId type;
};

struct FsmTransition
{
    FsmTransition(StringId eventType_, StringId newState_, const std::function<void()>& onTransition_)
        : eventType(eventType_),
        newState(newState_),
        onTransition(onTransition_)
    {
        
    }

    StringId eventType;
    StringId newState;
    std::function<void()> onTransition;
};

struct FsmState
{
    FsmState& AddTransition(StringId eventType, StringId newState, const std::function<void()>& onTransition)
    {
        transitions.emplace_back(eventType, newState, onTransition);

        return *this;
    }

    FsmState& AddTransition(StringId eventType, StringId newState)
    {
        return AddTransition(eventType, newState, nullptr);
    }

    FsmState& OnEnter(std::function<void()>& onEnter_)
    {
        onEnter = onEnter_;
        return *this;
    }

    FsmState& OnExit(std::function<void()>& onExit_)
    {
        onEnter = onExit_;
        return *this;
    }

    bool IsValidTransition(FsmEvent ev)
    {
        return FindTransition(ev) != nullptr;
    }

    static StringId DoTransition(FsmTransition* transition)
    {
        if(transition->onTransition)
        {
            transition->onTransition();
        }

        return transition->newState;
    }

    FsmTransition* FindTransition(FsmEvent ev)
    {
        for(auto& transition : transitions)
        {
            if(transition.eventType == ev.type)
            {
                return &transition;
            }
        }

        return nullptr;
    }

    StringId state;
    std::vector<FsmTransition> transitions;

    std::function<void()> onEnter;
    std::function<void()> onExit;
};

struct FiniteStateMachine
{
    FsmState& AddState(StringId stateName)
    {
        if(states.count(stateName.key) > 0)
        {
            FatalError("Duplicate fsm key: %s\n", stateName.ToString());
        }

        states[stateName.key] = std::make_unique<FsmState>();
        return *states[stateName.key].get();
    }

    StringId State() const
    {
        return currentState->state;
    }

    void SendEvent(FsmEvent ev)
    {
        auto transition = currentState->FindTransition(ev);

        if(transition != nullptr)
        {
            auto newState = FsmState::DoTransition(transition);
            SwitchState(newState);
        }
    }

    void SwitchState(StringId newStateName)
    {
        if(currentState->onExit != nullptr)
        {
            currentState->onExit();
        }

        auto newState = FindState(newStateName);
        currentState = newState;

        if(newState->onEnter != nullptr)
        {
            newState->onEnter();
        }
    }

    void SetInitialState(StringId stateName)
    {
        currentState = FindState(stateName);

        if(currentState->onEnter != nullptr)
        {
            currentState->onEnter();
        }
    }

    FsmState* FindState(StringId stateName)
    {
        auto state = states.find(stateName.key);
        if(state == states.end())
        {
            FatalError("Invalid fsm state: %s", stateName.ToString());
        }
        else
        {
            return state->second.get();
        }
    }

    std::unordered_map<unsigned int, std::unique_ptr<FsmState>> states;
    FsmState* currentState = nullptr;
};