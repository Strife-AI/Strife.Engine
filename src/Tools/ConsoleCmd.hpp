#pragma once

#include <vector>
#include <string>
#include <map>

#include "ConsoleValueParsing.hpp"

struct InvalidConsoleCommandCall : std::exception
{

};

class Console;
class Scene;

class ConsoleCommandBinder
{
public:
    // The first argument is always the command name
    ConsoleCommandBinder(const std::vector<std::string>& args, bool showHelp, Console* console)
        : _args(args),
        _showHelp(showHelp),
        _console(console)
    {

    }

    template<typename TParameter>
    ConsoleCommandBinder& Bind(TParameter& outValue, const std::string& name);

    template<typename TParameter>
    bool TryBind(TParameter& outValue, const std::string& name, bool optional = true);
   
    void Help(const std::string& description);
    void Error(const std::string& message);

    bool Success() const { return !_error; }
    std::string ErrorMessage() const { return _errorMessage; }
    std::string Usage() const { return _usage; }
    std::string Result() const { return _result; }
    Console* GetConsole() const { return _console; }
    const std::vector<std::string>& Args() const { return _args; }

private:
    std::vector<std::string> _args;
    std::string _usage;
    std::string _errorMessage;
    std::string _result;

    bool _error = false;
    int _currentArg = 1;
    bool _showHelp;

    Console* _console;
};

template<typename TParameter>
ConsoleCommandBinder &ConsoleCommandBinder::Bind(TParameter &outValue, const std::string &name)
{
    TryBind(outValue, name, false);
    
    return *this;
}

template <typename TParameter>
bool ConsoleCommandBinder::TryBind(TParameter& outValue, const std::string& name, bool optional)
{
    if (_currentArg != 1)
    {
        _usage += " ";
    }

    if(optional)
    {
        _usage += "(optional)[" + name + "]";
    }
    else
    {
        _usage += "[" + name + "]";
    }

    if (_currentArg >= (int)_args.size())
    {
        if (!optional)
        {
            Error("Too few arguments");
        }

        return false;
    }

    if (!_showHelp && !_error)
    {
        if (ParseConsoleValue(outValue, _args[_currentArg]) != ConsoleValueParseResult::Success)
        {
            Error("Could not bind parameter " + name);
            return false;
        }
    }

    ++_currentArg;

    return true;
}

struct ConsoleCmd
{
    ConsoleCmd(const std::string& name_, void (*commandHandler_)(ConsoleCommandBinder& binder));

    const std::string name;
    void (*commandHandler)(ConsoleCommandBinder& binder);
};

ConsoleCmd* GetConsoleCmd(const std::string &name);
void RegisterConsoleCmd(ConsoleCmd* cmd);
std::map<std::string, ConsoleCmd*>& GetAllConsoleCommands();