#include "FileResource.hpp"
#include "System/FileSystem.hpp"

bool FileResource::LoadFromFile(const ResourceSettings& settings)
{
    return TryReadFileContents(settings.path, _bytes);
}
