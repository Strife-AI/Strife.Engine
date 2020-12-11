#pragma once

#include <cstddef>

#include "gsl/span"

#include "ResourceEntryHeader.hpp"
#include "BinaryStreamReader.hpp"

class ResourceFileEntry
{
public:
	ResourceFileEntry(const ResourceEntryHeader& header_)
		: header(header_)
	{

	}

	ResourceEntryHeader header;
};

class ResourceFileReader
{
public:
    ResourceFileReader(BinaryStreamReader& reader)
		: _reader(reader)
    {
        
    }

    std::vector<ResourceFileEntry> LoadEntries();
    gsl::span<std::byte> LoadResource(const ResourceFileEntry& entry);

	void Close()
	{
		_reader.Close();
	}

private:
	BinaryStreamReader& _reader;
};