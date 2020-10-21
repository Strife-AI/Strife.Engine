#include <vector>
#include <cstring>

#include "ResourceFileReader.hpp"

#include "Memory/Cipher.hpp"

std::vector<ResourceFileEntry> ResourceFileReader::LoadEntries()
{
    char buf[5] = { 0 };

    _reader.ReadBlob(buf, 4);

    if (strcmp(buf, "STRF") != 0)
    {
        FatalError("Invalid resource file");
    }

    if (_reader.ReadInt() != CONTENT_VERSION)
    {
        FatalError("Content file incompatible with game. Please update game and content.");
    }

    auto cipher = GetDefaultCipher();
    _reader.DecryptRange(8, _reader.TotalSize(), cipher);

    int headerOffset = _reader.ReadInt();

    _reader.Seek(headerOffset);
    int totalEntries = _reader.ReadInt();

    std::vector<ResourceFileEntry> entries;

    for (int i = 0; i < totalEntries; ++i)
    {
        ResourceEntryHeader header;
        header.key = _reader.ReadInt();
        header.type = _reader.ReadInt();
        header.offset = _reader.ReadInt();

        entries.emplace_back(header);
    }

    return entries;
}

gsl::span<std::byte> ResourceFileReader::LoadResource(const ResourceFileEntry& entry)
{
    _reader.Seek(entry.header.offset);

    unsigned int correctCrc32 = _reader.ReadInt();

    int size = _reader.ReadInt();
    unsigned int typeKey = entry.header.type;

    auto data = new std::byte[size];
    _reader.ReadBlob(data, size);

    unsigned int computedCrc32 = Crc32(reinterpret_cast<const char*>(&size), 4);
    computedCrc32 = Crc32AddBytes(computedCrc32, reinterpret_cast<const char*>(&typeKey), 4);
    computedCrc32 = Crc32AddBytes(computedCrc32, reinterpret_cast<const char*>(data), size);

    if (computedCrc32 != correctCrc32)
    {
        FatalError("Corrupt resource %u, got %u expected %u", entry.header.key, computedCrc32, correctCrc32);
    }

    return gsl::span<std::byte>(data, data + size);
}
