#include <string>

#include "ResourceFileWriter.hpp"
#include "FileSystem.hpp"
#include "Logger.hpp"
#include "Memory/Cipher.hpp"

void ResourceFileWriter::Initialize(const char* outputFileName)
{
	_outputFileName = outputFileName;
	_writer.WriteBlob("STRF", 4);
	_writer.WriteInt(0);
	_writer.WriteInt(0); // Reserve space for header offset in file
}

bool ResourceFileWriter::HasResource(const std::string& name) const
{
	return _resources.count(name) != 0;
}

StringId ResourceFileWriter::WriteResource(void* data, int size, const std::string& name, const std::string& typeName)
{
	if (HasResource(name))
	{
		FatalError("Duplicate asset: %s\n", name.c_str());
	}

	StringId key(name.c_str());

	if (_keysByName.count(name) != 0 && _keysByName[name] != key)
	{
		FatalError(
			"Duplicate StringIds with different strings :O (%s collides with %s)",
			name.c_str(),
			_keysByName[name].ToString());
	}

	unsigned int typeKey = StringId(typeName.c_str()).key;

	ResourceEntryHeader header;
	header.name = name;
	header.key = key.key;
	header.offset = _writer.CurrentPosition();
	header.type = typeKey;

	unsigned int crc32 = Crc32(reinterpret_cast<const char*>(&size), 4);
	crc32 = Crc32AddBytes(crc32, reinterpret_cast<const char*>(&typeKey), 4);
	crc32 = Crc32AddBytes(crc32, reinterpret_cast<const char*>(data), size);

	_writer.WriteInt(crc32);
	_writer.WriteInt(size);

	_resources[name] = header;
	_keysByName[name] = key;

	_writer.WriteBlob(data, size);

	return key;
}

StringId ResourceFileWriter::WriteResourceFile(
	const std::string& fileName,
	const std::string& name,
	const std::string& typeName)
{
	if (_keysByName.count(name) != 0)
	{
		return _keysByName[name];
	}

	FILE* file = OpenFile(fileName.c_str(), "rb");
	if (file == nullptr)
	{
		FatalError("Failed to open %s for reading", fileName.c_str());
	}

	fseek(file, 0, SEEK_END);
	int size = ftell(file);

	fseek(file, 0, SEEK_SET);

	auto buf = new char[size];
	fread(buf, 1, size, file);

	auto key = WriteResource(buf, size, name, typeName);

	delete[] buf;

	fclose(file);

	return key;
}

void ResourceFileWriter::Finish()
{
	WriteHeader();
	auto cipher = GetDefaultCipher();

	_writer.EncryptRange(8, _writer.TotalSize(), cipher);
	_writer.WriteToFile(_outputFileName);
}

StringId ResourceFileWriter::GetKeyByName(const std::string& name)
{
	return _keysByName[name];
}

void ResourceFileWriter::WriteHeader()
{
	int headerPosition = _writer.CurrentPosition();
	_writer.WriteInt(_resources.size());

	for (auto& resource : _resources)
	{
		_writer.WriteInt(resource.second.key);
		_writer.WriteInt(resource.second.type);
		_writer.WriteInt(resource.second.offset);
	}

	// Write header starting position at position 8 of file (we already reserved space for it)
	_writer.Seek(8);
	_writer.WriteInt(headerPosition);
}
