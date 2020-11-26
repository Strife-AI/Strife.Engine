#pragma once
#include <memory>
#include <optional>

struct IThreadPoolWorkItem
{
    virtual ~IThreadPoolWorkItem() = default;

    void Run()
    {
        Execute();
        _isComplete = true;
    }

    bool IsComplete() const
    {
        return _isComplete;
    }

private:
    virtual void Execute() = 0;

    bool _isComplete = false;
};

template<typename TResult>
struct ThreadPoolWorkItem : IThreadPoolWorkItem
{
    bool TryGetResult(const TResult& outResult)
    {
        if(IsComplete() && _result.has_value())
        {
            outResult = _result.value();
            return true;
        }
        else
        {
            return false;
        }
    }

protected:
    std::optional<TResult> _result;
};

class ThreadPool
{
public:
    void StartItem(const std::shared_ptr<IThreadPoolWorkItem>& item) { }

    static ThreadPool* GetInstance() { return nullptr; }    // TODO

private:

};