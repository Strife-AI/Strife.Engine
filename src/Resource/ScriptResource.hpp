#pragma once

#include "ResourceManager.hpp"
#include "Scripting/Scripting.hpp"
#include "Thread/SpinLock.hpp"

struct ScriptResource : BaseResource
{
    bool LoadFromFile(const ResourceSettings& settings) override;
    std::shared_ptr<Script> CreateScript();

    std::string source;
    int currentVersion = 0;
};