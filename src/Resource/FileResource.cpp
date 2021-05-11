#include "FileResource.hpp"
#include "System/FileSystem.hpp"
#include "ResourceSettings.hpp"

bool FileResource::LoadFromFile(const ResourceSettings& settings)
{
    return TryReadFileContents(settings.path, _bytes);
}

bool FileResource::TryCleanup()
{
    _bytes.clear();
    return true;
}
