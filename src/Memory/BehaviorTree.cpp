#include "BehaviorTree.hpp"

#include "Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneManager.hpp"

// #define DEBUG_TREE

bool debugTree = false;

BehaviorStatus Behavior::Tick()
{
#ifdef DEBUG_TREE
    static int indent = 0;

    if (debugTree)
    {


        for (int i = 0; i < indent; ++i) printf("\t");

        printf("- Tick %s", typeid(*this).name());

        if(auto condition = dynamic_cast<ConditionBehavior*>(this))
        {
            if (condition->debugString != nullptr)
            {
                printf("(%s)", condition->debugString);
            }
        }

        printf("\n");
    }

    ++indent;

#endif

    if (_status != BehaviorStatus::Running)
    {
        OnStart();
    }

    _status = Update();

    if (_status != BehaviorStatus::Running)
    {
        OnStop(_status);
    }

#ifdef DEBUG_TREE
    --indent;
#endif

    return _status;
}

void WaitBehavior::OnStart()
{
    timerStart = GetTime();
}

BehaviorStatus WaitBehavior::Update()
{
    return GetTime() >= timerStart + length
        ? BehaviorStatus::Success
        : BehaviorStatus::Running;
}

float WaitBehavior::GetTime()
{
    FatalError("Not implemented");
    //return Engine::GetInstance()->GetSceneManager()->GetScene()->relativeTime;
}

void CompositeBehavior::OnStop(BehaviorStatus status)
{
    if (status == BehaviorStatus::Aborted)
    {
        for (auto child : _children)
        {
            if (child->GetStatus() == BehaviorStatus::Running)
            {
                child->Abort();
            }
        }
    }
}

CompositeBehavior::~CompositeBehavior()
{
    for (auto child : _children)
    {
        delete child;
    }
}

void SequenceBehavior::OnStart()
{
    _activeChild = _children.begin();
}

BehaviorStatus SequenceBehavior::Update()
{
    for (; _activeChild != _children.end(); ++_activeChild)
    {
        auto status = (*_activeChild)->Tick();

        if (status != BehaviorStatus::Success)
        {
            return status;
        }
    }

    return BehaviorStatus::Success;
}

BehaviorStatus ConditionBehavior::Update()
{
    return condition()
        ? BehaviorStatus::Success
        : (wait
            ? BehaviorStatus::Running
            : BehaviorStatus::Failure);
}

BehaviorStatus ParallelBehavior::Update()
{
    int successCount = 0;
    int failureCount = 0;

    for (auto behavior : _children)
    {
        behavior->Tick();

        if (behavior->GetStatus() == BehaviorStatus::Success)
        {
            ++successCount;
            if (_successPolicy == RequireOne)
            {
                return BehaviorStatus::Success;
            }
        }

        if (behavior->GetStatus() == BehaviorStatus::Failure)
        {
            ++failureCount;
            if (_failurePolicy == RequireOne)
            {
                return BehaviorStatus::Failure;
            }
        }
    }

    if (_failurePolicy == RequireAll && failureCount == _children.size())
    {
        return BehaviorStatus::Failure;
    }

    if (_successPolicy == RequireAll && successCount == _children.size())
    {
        return BehaviorStatus::Success;
    }

    return BehaviorStatus::Running;
}

void ParallelBehavior::OnStart()
{

}

void ParallelBehavior::OnStop(BehaviorStatus status)
{
    for (auto behavior : _children)
    {
        if (behavior->GetStatus() == BehaviorStatus::Running)
        {
            behavior->Abort();
        }
    }
}

void SelectorBehavior::OnStart()
{
    _activeBehavior = _children.begin();
}

BehaviorStatus SelectorBehavior::Update()
{
    for (; _activeBehavior != _children.end(); ++_activeBehavior)
    {
        auto childStatus = (*_activeBehavior)->Tick();

        if (childStatus != BehaviorStatus::Failure)
        {
            return childStatus;
        }
    }

    return BehaviorStatus::Failure;
}

void ActiveSelectorBehavior::OnStart()
{
    _activeBehavior = _children.end();
}

BehaviorStatus ActiveSelectorBehavior::Update()
{
    auto previousActive = _activeBehavior;

    SelectorBehavior::OnStart();
    auto result = SelectorBehavior::Update();

    if (previousActive != _children.end() && _activeBehavior != previousActive)
    {
        (*previousActive)->Abort();
    }

    return result;
}


