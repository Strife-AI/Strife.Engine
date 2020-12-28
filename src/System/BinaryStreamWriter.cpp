#include <cstdio>
#include <cstring>

#include "BinaryStreamWriter.hpp"
#include "FileSystem.hpp"
#include "Memory/Cipher.hpp"
#include "Logger.hpp"

void BinaryStreamWriter::WriteInt(int value)
{
	WriteBlob(&value, sizeof(int));
}

void BinaryStreamWriter::WriteFloat(float value)
{
    WriteBlob(&value, sizeof(float));
}

void BinaryStreamWriter::WriteBlob(const void* data, int size)
{
	ExpandBuffer(size);

    const auto bytes = static_cast<const unsigned char*>(data);
	memcpy(&_data[_currentPosition], data, size);

	_currentPosition += size;
}

void BinaryStreamWriter::Seek(int offset)
{
	_currentPosition = offset;
}

int BinaryStreamWriter::CurrentPosition() const
{
	return _currentPosition;
}

void BinaryStreamWriter::WriteToFile(const char* fileName)
{
	auto file = OpenFile(fileName, "wb");

	if (file == nullptr)
	{
		FatalError("Failed to open %s for writing", fileName);
	}

	fwrite(&_data[0], 1, _data.size(), file);
	fclose(file);
}

void BinaryStreamWriter::EncryptRange(int startInclusive, int endExclusive, Cipher& cipher)
{
	Assert(startInclusive >= 0 && startInclusive < _data.size());
	Assert(endExclusive >= startInclusive && endExclusive <= _data.size());

	cipher.Encrypt((byte*)_data.data() + startInclusive, (byte*)_data.data() + endExclusive);
}

void BinaryStreamWriter::ExpandBuffer(int newMinSize)
{
	while ((int)_data.size() < _currentPosition + newMinSize)
	{
		_data.push_back(0);
	}
}
