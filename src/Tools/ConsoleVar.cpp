#include <map>
#include <System/Logger.hpp>

#include "ConsoleVar.hpp"
#include "ConsoleCmd.hpp"

std::map<std::string, IConsoleVar*>& GetAllConsoleVars()
{
    static std::map<std::string, IConsoleVar*> g_consoleVarsByName;

    return g_consoleVarsByName;
}

IConsoleVar* GetConsoleVar(const std::string &name)
{
    auto& allVars = GetAllConsoleVars();
    auto find = allVars.find(name);

    return find != allVars.end()
        ? find->second
        : nullptr;
}

void RegisterConsoleVar(IConsoleVar* var)
{
    auto& allVars = GetAllConsoleVars();
    auto find = allVars.find(var->name);

    if(find == allVars.end())
    {
        allVars[var->name] = var;
    }
    else
    {
        FatalError("Duplicate console var: %s", var->name.c_str());
    }
}
