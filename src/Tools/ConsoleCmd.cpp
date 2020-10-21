#include <System/Logger.hpp>

#include "ConsoleCmd.hpp"
#include "Console.hpp"
#include "Engine.hpp"
#include "Scene/SceneManager.hpp"

void ConsoleCommandBinder::Help(const std::string& description)
{
    if(_currentArg < _args.size())
    {
        Error("Too many arguments");
    }

    if(_error || _showHelp)
    {
        _usage = "Usage: " + _args[0] + " " + _usage + "\nDescription: " + description;

        if(_showHelp)
        {
            GetConsole()->Log(_usage.c_str());
        }

        // Prevent the command from actually running
        throw InvalidConsoleCommandCall();
    }
}

void ConsoleCommandBinder::Error(const std::string& message)
{
    _error = true;
    _errorMessage = message;
}

Scene* ConsoleCommandBinder::GetScene()
{
    return Engine::GetInstance()->GetSceneManager()->GetScene();
}

static std::map<std::string, ConsoleCmd*>& GetAllConsoleCmds()
{
    static std::map<std::string, ConsoleCmd*> g_consoleCmdsByName;

    return g_consoleCmdsByName;
}

std::map<std::string, ConsoleCmd*>& GetAllConsoleCommands()
{
    return GetAllConsoleCmds();
}

ConsoleCmd* GetConsoleCmd(const std::string &name)
{
    auto& allCmds = GetAllConsoleCmds();
    auto find = allCmds.find(name);

    return find != allCmds.end()
           ? find->second
           : nullptr;
}

void RegisterConsoleCmd(ConsoleCmd* cmd)
{
    auto& allCmds = GetAllConsoleCmds();
    auto find = allCmds.find(cmd->name);

    if(find == allCmds.end())
    {
        allCmds[cmd->name] = cmd;
    }
    else
    {
        FatalError("Duplicate console cmd: %s", cmd->name.c_str());
    }
}

ConsoleCmd::ConsoleCmd(const std::string& name_, void (* commandHandler_)(ConsoleCommandBinder&))
    : name(name_),
      commandHandler(commandHandler_)
{
    RegisterConsoleCmd(this);
}
