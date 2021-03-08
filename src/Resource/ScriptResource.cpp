#include "ScriptResource.hpp"
#include "System/FileSystem.hpp"

bool ScriptResource::LoadFromFile(const ResourceSettings& settings)
{
    bool success = TryReadFileContents(settings.path, source);
    if (success)
    {
        ++currentVersion;
    }

    return success;
}

std::shared_ptr<Script> ScriptResource::CreateScript()
{
    return std::make_shared<Script>(this);
}

