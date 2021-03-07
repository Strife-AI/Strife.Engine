#include "ScriptCompiler.hpp"
#include "Resource/ScriptResource.hpp"

void ScriptCompiler::RequestCompile(std::shared_ptr<Script> script)
{
    _scriptQueue.Enqueue(script);

    // Prevent blocking forever if called from main thread
    if (std::this_thread::get_id() == _gameThreadId)
    {
        Update();
    }
}

// NOTE: this MUST be executed on the main game thread! There are no locks preventing script source modification.
void ScriptCompiler::Update()
{
    std::shared_ptr<Script> script;
    while (_scriptQueue.TryDequeue(script))
    {
        script->_compilationSuccessful = script->Compile("script", script->_scriptResource->source.c_str());
        script->_compilationDone = true;
    }
}

ScriptCompiler::ScriptCompiler()
    : _gameThreadId(std::this_thread::get_id())
{

}

ScriptCompiler* ScriptCompiler::GetInstance()
{
    static ScriptCompiler compiler;
    return &compiler;
}
