#pragma once

#include "Scripting.hpp"
#include "Container/ConcurrentQueue.hpp"

class ScriptCompiler
{
public:
    ScriptCompiler();

    void RequestCompile(std::shared_ptr<Script> script);
    void Update();

    static ScriptCompiler* GetInstance();

private:
    ConcurrentQueue<std::shared_ptr<Script>> _scriptQueue;
    std::thread::id _gameThreadId;
};