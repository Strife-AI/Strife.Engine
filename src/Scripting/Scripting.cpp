#include <vector>
#include <libtcc.h>

#include "Scripting.hpp"

static std::vector<ScriptCallableInfo*>& GetAllScriptCallableFunctions()
{
    static std::vector<ScriptCallableInfo*> functions;
    return functions;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void ScriptCallableInfo::Initialize(const std::string_view& functionPointerPrototype)
{
    prototype = functionPointerPrototype;
    replace(prototype, "(*)", name);

    GetAllScriptCallableFunctions().push_back(this);
}

void* Script::GetSymbolOrNull(const char* name)
{
    return tcc_get_symbol(_tccState, name);
}
