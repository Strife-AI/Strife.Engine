#pragma once

#include <map>
#include <string>

#include "Memory/StringId.hpp"
#include "System/BinaryStreamWriter.hpp"
#include "ResourceEntryHeader.hpp"

class ResourceFileWriter
{
public:
    void Initialize(const char* outputFileName);
    bool HasResource(const std::string& name) const;
    StringId WriteResource(void* data, int size, const std::string& name, const std::string& typeName);
    StringId WriteResourceFile(const std::string& fileName, const std::string& name, const std::string& typeName);
    void Finish();
    StringId GetKeyByName(const std::string& name);

private:
    void WriteHeader();

	BinaryStreamWriter _writer;
	std::map<std::string, ResourceEntryHeader> _resources;
	std::map<std::string, StringId> _keysByName;
    const char* _outputFileName;
};
