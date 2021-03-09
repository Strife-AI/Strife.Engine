#pragma once

#include <string_view>
#include <string>
#include <memory>
#include <type_traits>
#include <csetjmp>
#include <thread>
#include <functional>

#include "System/Logger.hpp"

template <typename T>
constexpr auto type_name() noexcept {
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
  prefix = "auto type_name() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
  prefix = "auto __cdecl type_name<";
  suffix = ">(void) noexcept";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}

struct ThreadState
{
    jmp_buf errorHandler;
};

ThreadState* GetThreadState(std::thread::id threadId);

template<typename TFunc, TFunc func>
const char* ScriptCallableName;

template<typename Fn, Fn fn, typename... Args>
typename std::result_of<Fn(Args...)>::type wrapper(Args... args)
{
    try
    {
        return fn(std::forward<Args>(args)...);
    }
    catch(const std::exception& e)
    {
        // Prevent exceptions from going through the C code, which would crash
        // Instead, do a longjmp to abort the script
        Log("Aborting call to %s because got instance of %s: %s\n", ScriptCallableName<Fn, fn>,  typeid(e).name(), e.what());
        auto threadState = GetThreadState(std::this_thread::get_id());
        longjmp(threadState->errorHandler, 1);
    }
}

#define WRAPIT(FUNC) wrapper<decltype(&FUNC), &FUNC>

struct ScriptCallableInfo
{
    template<typename TFunc>
    ScriptCallableInfo(const char* name, TFunc func)
        : name(name),
        functionPointer((void*)func)
    {
        auto functionPointerPrototype = type_name<decltype(func)>();
        Initialize(functionPointerPrototype);
    }

    std::string prototype;
    const char* name;
    void* functionPointer;

private:
    void Initialize(const std::string_view& functionPointerPrototype);
};

template<typename TFunc>
class ScriptFunction;

template<typename TReturnType, typename ... Args>
class ScriptFunction<TReturnType(Args...)>
{
public:
    ScriptFunction(const char* name)
        : _name(name)
    {

    }

    const char* Name() const
    {
        return _name;
    }

    auto operator()(Args... args)
    {
        if (_functionPointer == nullptr)
        {
            throw std::bad_function_call();
        }

        auto threadState = GetThreadState(std::this_thread::get_id());
        if (setjmp(threadState->errorHandler) == 0)
        {
            return _functionPointer(args...);
        }
        else
        {
            Log("Call to %s failed\n", _name);
            throw std::bad_function_call();
        }
    }

private:
    TReturnType (*_functionPointer)(Args...) = nullptr;
    const char* _name;

    friend class Script;
};

struct ScriptResource;

class Script : public std::enable_shared_from_this<Script>
{
public:
    Script(ScriptResource* scriptResource);

    template<typename TFunc>
    bool TryBindFunction(ScriptFunction<TFunc>& outFunction);
    bool TryCompile();
    bool TryRecompileIfNewer();

private:
    void* GetSymbolOrNull(const char* name);
    bool Compile(const char* name, const char* source);

    friend class ScriptCompiler;

    struct TCCState* _tccState = nullptr;
    ScriptResource* _scriptResource;
    int _currentScriptVersion = -1;
    bool _compilationDone = false;
    bool _compilationSuccessful = false;
};

template<typename TFunc>
bool Script::TryBindFunction(ScriptFunction<TFunc>& outFunction)
{
    outFunction._functionPointer = (decltype(outFunction._functionPointer))GetSymbolOrNull(outFunction.Name());
    return outFunction._functionPointer != nullptr;
}

#define SCRIPT_REGISTER(name_) static ScriptCallableInfo g_functioninfo_##name_(#name_, (decltype(&name_))WRAPIT(name_)); \
ScriptCallableName<decltype(&name_), name_> = #name_;
