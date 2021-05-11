#include "ScriptResource.hpp"
#include "System/FileSystem.hpp"
#include "ResourceSettings.hpp"

bool ScriptResource::LoadFromFile(const ResourceSettings& settings)
{
    bool success = TryReadFileContents(settings.path, source.source);
    if (success)
    {
        ++source.currentVersion;
    }

    return success;
}