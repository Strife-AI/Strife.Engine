#pragma once

#include <string_view>
#include <string>
#include <memory>
#include <type_traits>
#include <setjmp.h>

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

struct ScriptCallableInfo
{
    template<typename TFunc>
    ScriptCallableInfo(const char* name, TFunc func)
        : name(name),
        functionPointer(static_cast<void*>(func))
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

class Script : std::enable_shared_from_this<Script>
{
public:
    template<typename TFunc>
    bool TryBindFunction(const char* name, TFunc& outFunc);

private:
    void* GetSymbolOrNull(const char* name);

    struct TCCState* _tccState;
};

//template<typename TReturnType, typename... Args>
//bool ScriptFunction<TReturnType, Args...>::TryInvokeWithReturnValue(TReturnType& outReturnValue, Args&& ... args)
//{
//    static_assert(!std::is_same_v<TReturnType, void>);
//
//    if (_func == nullptr)
//    {
//        return false;
//    }
//    else
//    {
//        outReturnValue = _func(args...);
//        return true;
//    }
//}

class ScriptCompiler
{
public:
    bool CompileFromSource(const char* source);

private:
};

template<typename TFunc>
bool Script::TryBindFunction(const char* name, TFunc& outFunc)
{
    outFunc = GetSymbolOrNull(name);
    return outFunc != nullptr;
}

#define SCRIPT_CALLABLE(returnType_, name_, params_...) returnType_ name_(params_); \
    static ScriptCallableInfo g_functioninfo_##name_ { #name_, name_ }; \
    returnType_ name_(params_)

#define SCRIPT_FUNCTION(returnType_, name_, params_...) decltype(returnType_) //ScriptFunction<returnType_, params_> name_(#name_)