#pragma once

#include <functional>
#include <vector>


#include "FixedSizeVector.hpp"
#include "StringId.hpp"

enum class BehaviorStatus
{
    Invalid,    // Initial state
    Running,    // The node is currently running
    Success,    // The node successfully executed
    Failure,    // The node failed to execute
    Aborted     // The node was aborted because a higher priority node started executing
};

struct Behavior
{
    virtual ~Behavior() = default;

    BehaviorStatus Tick();

    void Abort()
    {
        _status = BehaviorStatus::Aborted;
        OnStop(BehaviorStatus::Aborted);
    }

    BehaviorStatus GetStatus() const
    {
        return _status;
    }

    // Called when a node starts executing before the first call to Update()
    virtual void OnStart() { }

    // Called after the last update or when aborted. The status indicates why it stopped.
    virtual void OnStop(BehaviorStatus status)  { }

    void Stop(BehaviorStatus status)
    {
        _status = status;
        OnStop(status);
    }

private:
    virtual BehaviorStatus Update() = 0;

    BehaviorStatus _status = BehaviorStatus::Invalid;
};

// Waits for the specified period of time to pass
struct WaitBehavior : Behavior
{
    WaitBehavior(float lengthSeconds)
        : length(lengthSeconds)
    {
        
    }

    void OnStart() override;
    BehaviorStatus Update() override;

    static float GetTime();

    float timerStart;
    float length;
};

// Represents a group of behaviors. Aborting a composite behavior also aborts all of its running children.
struct CompositeBehavior : Behavior
{
    CompositeBehavior() = default;

    CompositeBehavior(const std::initializer_list<Behavior*> children)
        : _children(children)
    {
        
    }

    virtual ~CompositeBehavior();

    void OnStop(BehaviorStatus status) override;

    void AddChild(Behavior* behavior)
    {
        _children.push_back(behavior);
    }

protected:
    std::vector<Behavior*> _children;
};

// A group of behaviors run in consecutive order. If a child behavior returns:
//      * SUCCESS: the next one is immediately executed
//      * FAILURE: sequence stops and returns failure
//      * RUNNING: the sequence stops and returns running. On the next tick, it will pick up where it left off
// Aborting a sequence aborts all of its children.
struct SequenceBehavior : CompositeBehavior
{
    SequenceBehavior() = default;
    SequenceBehavior(const std::initializer_list<Behavior*> children)
        : CompositeBehavior(children)
    {
        
    }

    void OnStart() override;
    BehaviorStatus Update() override;

private:
    std::vector<Behavior*>::iterator _activeChild;
};

// A behavior for checking conditions. The update returns:
//      * SUCCESS if the condition is true
//      * FAILURE if the condition is false, but wait is false
//      * RUNNING if the condition is false, but wait is true
struct ConditionBehavior : Behavior
{
    ConditionBehavior(const std::function<bool()>& condition_, bool wait_ = false, const char* debugString_ = nullptr)
        : condition(condition_),
        wait(wait_),
        debugString(debugString_)
    {
        
    }

    BehaviorStatus Update() override;

    std::function<bool()> condition;
    const char* debugString = nullptr;
    bool wait;
};

// Runs multiple behaviors in parallel (but *not* on different threads). The children are actually executed sequentially,
// but unlike a sequence, a failure/running state in a previous child does not necessarily cause it to stop running.
// Thus, there can be multiple nodes in the running state at the same time, whereas a sequence would stop and wait.
// The policy dictates what should be considered a success, and what should be considered a failure e.g. do you
// require every node to fail to be considered a failure, or just one? If just one, it will early terminate if
// that condition is met.
struct ParallelBehavior : CompositeBehavior
{
public:
    enum Policy
    {
        RequireOne,
        RequireAll,
    };

    ParallelBehavior() = default;
    ParallelBehavior(Policy forSuccess, Policy forFailure, const std::initializer_list<Behavior*> children)
        : CompositeBehavior(children),
        _successPolicy(forSuccess),
        _failurePolicy(forFailure)
    {

    }

    ParallelBehavior(Policy forSuccess, Policy forFailure)
        : _successPolicy(forSuccess),
        _failurePolicy(forFailure)
    {

    }

    virtual ~ParallelBehavior() = default;

protected:
    Policy _successPolicy;
    Policy _failurePolicy;

    BehaviorStatus Update() override;

    void OnStart() override;
    void OnStop(BehaviorStatus status) override;
};

// Runs a set of conditions and actions in parallel. The conditions are checked every tick. If the conditions fail, all
// running children are aborted.
struct MonitorBehavior : ParallelBehavior
{
    MonitorBehavior()
        : ParallelBehavior(RequireAll, RequireOne)
    {
        
    }

    MonitorBehavior(const std::initializer_list<ConditionBehavior*> conditions, const std::initializer_list<Behavior*> actions)
        : ParallelBehavior(RequireAll, RequireOne)
    {
        for(auto condition : conditions)
        {
            AddCondition(condition);
        }

        for(auto action : actions)
        {
            AddAction(action);
        }
    }

    void AddCondition(Behavior* condition)
    {
        _children.insert(_children.begin(), condition);
    }

    void AddAction(Behavior* action)
    {
        _children.push_back(action);
    }
};

// Tries to find a suitable behavior that can be executed. It's used to implement OR logic (this action OR that action).
// It runs multiple behaviors sequentially until a single child is not a failure. If that child is running, on the next
// tick, it will resume running that node.
//
// The nodes should be put in order of priority, so highest priority actions get picked first
struct SelectorBehavior : CompositeBehavior
{
    SelectorBehavior() = default;
    SelectorBehavior(const std::initializer_list<Behavior*>& children)
        : CompositeBehavior(children)
    {
        
    }

    virtual ~SelectorBehavior() = default;

    void OnStart() override;
    BehaviorStatus Update() override;

protected:
    std::vector<Behavior*>::iterator _activeBehavior;
};

// Same as a selector, but the currently executing child can be aborted if a higher priority child can be successfully executed.
// Note: the previously executing child is aborted *after* the new node starts executing!
struct ActiveSelectorBehavior : SelectorBehavior
{
    ActiveSelectorBehavior() = default;
    ActiveSelectorBehavior(const std::initializer_list<Behavior*>& children)
        : SelectorBehavior(children)
    {

    }

    void OnStart() override;
    BehaviorStatus Update() override;
};

// Continues to return RUNNING status until aborted by another node.
struct BlockUntilAbortedBehavior : SelectorBehavior
{
    BehaviorStatus Update() override
    {
        return BehaviorStatus::Running;
    }
};

// Prints a debug message to the console on start and returns success.
struct DebugMessageBehavior : Behavior
{
    DebugMessageBehavior(const std::string& message_)
        : message(message_)
    {
        
    }

    void OnStart() override
    {
        printf("%s\n", message.c_str());
    }

    BehaviorStatus Update() override
    {
        return BehaviorStatus::Success;
    }

    std::string message;
};

// Executes given lambda functions upon starting, updating, and exiting/aborting the node.
struct AdHocBehavior : Behavior
{
    AdHocBehavior(
        const std::function<void()>& onStart_,
        const std::function<BehaviorStatus()>& update_,
        const std::function<void(BehaviorStatus)>& onStop_)
            : onStart(onStart_),
            update(update_),
            onStop(onStop_)
    {
        
    }

    void OnStart() override
    {
        onStart();
    }

    BehaviorStatus Update() override
    {
        return update();
    }

    void OnStop(BehaviorStatus status) override
    {
        onStop(status);
    }

    std::function<void()> onStart;
    std::function<BehaviorStatus()> update;
    std::function<void(BehaviorStatus)> onStop;
};

// Executes a single lambda function upon start.
struct ActionBehavior : Behavior
{
    ActionBehavior(const std::function<void()>& action_)
        : action(action_)
    {
        
    }

    void OnStart() override
    {
        action();
    }

    BehaviorStatus Update() override
    {
        return BehaviorStatus::Success;
    }

    std::function<void()> action;
};

struct DecoratorBehavior : Behavior
{
    DecoratorBehavior(Behavior* behavior_)
        : behavior(behavior_)
    {
        
    }

    Behavior* behavior;
};

struct RepeatUntilAborted : DecoratorBehavior
{
    RepeatUntilAborted(Behavior* behavior_)
        : DecoratorBehavior(behavior_)
    {

    }

    BehaviorStatus Update() override
    {
        behavior->Tick();
        return BehaviorStatus::Running;
    }

    void OnStop(BehaviorStatus status) override
    {
        behavior->Stop(status);
    }
};

struct RepeatWhileBehavior : DecoratorBehavior
{
    RepeatWhileBehavior(const std::function<bool()>& condition_, Behavior* behavior_, bool checkFirstTime_ = true)
        : DecoratorBehavior(behavior_),
        condition(condition_),
        checkFirstTime(checkFirstTime_)
    {

    }

    void OnStart() override
    {
        firstTime = true;
    }

    BehaviorStatus Update() override
    {
        bool passFirstTime = firstTime && !checkFirstTime;

        firstTime = false;

        if (passFirstTime || condition())
        {
            behavior->Tick();
            return BehaviorStatus::Running;
        }
        else
        {
            if (behavior->GetStatus() == BehaviorStatus::Running)
                behavior->Abort();

            return BehaviorStatus::Success;
        }
    }

    void OnStop(BehaviorStatus status) override
    {
        behavior->Stop(status);
    }

    std::function<bool()> condition;
    bool checkFirstTime;
    bool firstTime = true;
};

struct NotifyOnCompleteBehavior : SequenceBehavior
{
    NotifyOnCompleteBehavior(const std::function<void(BehaviorStatus)>& onComplete_, const std::initializer_list<Behavior*> children)
        : SequenceBehavior(children),
        onComplete(onComplete_)
    {

    }

    void OnStop(BehaviorStatus status) override
    {
        SequenceBehavior::OnStop(status);
        onComplete(status);
    }

    std::function<void(BehaviorStatus)> onComplete;
};

struct BehaviorTree
{
    ~BehaviorTree()
    {
        delete root;
    }

    void SendEvent(StringId eventType)
    {
        events.PushBackIfRoom(eventType);
    }

    void Tick()
    {
        root->Tick();
        events.Clear();
    }

    bool ReceivedEvent(StringId type) const
    {
        return events.Contains(type);
    }

    Behavior* root = nullptr;
    FixedSizeVector<StringId, 32> events;
};

struct CheckForEventBehavior : Behavior
{
    CheckForEventBehavior(BehaviorTree* behaviorTree_, StringId eventType_, bool wait_ = false)
        : behaviorTree(behaviorTree_),
        eventType(eventType_),
        wait(wait_)
    {

    }

    BehaviorStatus Update() override
    {
        return behaviorTree->ReceivedEvent(eventType)
            ? BehaviorStatus::Success
            : wait ? BehaviorStatus::Running : BehaviorStatus::Failure;
    }

    BehaviorTree* behaviorTree;
    StringId eventType;
    bool wait;
};