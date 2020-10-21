#pragma once

#include <string>
#include <map>

#include "ConsoleValueParsing.hpp"

struct IConsoleVar
{
    IConsoleVar(const std::string& name_, bool persist_)
        : name(name_),
        persist(persist_)
    {

    }

    virtual ConsoleValueParseResult TrySetValue(const std::string& str) = 0;
    virtual std::string GetStringValue() = 0;

    std::string name;
    bool persist;
};

std::map<std::string, IConsoleVar*>& GetAllConsoleVars();
IConsoleVar* GetConsoleVar(const std::string& name);
void RegisterConsoleVar(IConsoleVar* var);

template<typename TVar>
class ConsoleVar final : public IConsoleVar
{
public:
    ConsoleVar(const std::string& name, const TVar& initialValue, bool persist = false);
    virtual ~ConsoleVar() = default;

    TVar& Value() { return _value; }

    const TVar& Value() const { return _value; }

    TVar& Default() { return _value; }

    const TVar& Default() const { return _value; }

    void SetValue(const TVar& value) { _value = value; }

    void ResetToDefault() { _value = _default; }

private:
    ConsoleValueParseResult TrySetValue(const std::string& str) override
    {
        return ParseConsoleValue(_value, str);
    }

    std::string GetStringValue() override
    {
        return SerializeConsoleValue(_value);
    }

    TVar _value;
    TVar _default;
};

template<typename TVar>
ConsoleVar<TVar>::ConsoleVar(const std::string &name, const TVar &initialValue, bool persist)
    : IConsoleVar(name, persist)
{
    RegisterConsoleVar(this);
	_value = initialValue;
	_default = initialValue;
}
