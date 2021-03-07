#include "ScriptResource.hpp"
#include "System/FileSystem.hpp"

bool ScriptResource::LoadFromFile(const ResourceSettings& settings)
{
    return TryReadFileContents(settings.path, source);
}

std::shared_ptr<Script> ScriptResource::CreateScript()
{
    return std::make_shared<Script>(this);
}

