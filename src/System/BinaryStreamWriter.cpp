#include <cstdio>
#include <cstring>

#include "BinaryStreamWriter.hpp"
#include "FileSystem.hpp"
#include "Memory/Cipher.hpp"
#include "Logger.hpp"

void BinaryStreamWriter::WriteInt(int value)
{
    WriteBlob({ reinterpret_cast<unsigned char*>(&value), sizeof(value) });
}

void BinaryStreamWriter::WriteFloat(float value)
{
    WriteBlob({ reinterpret_cast<unsigned char*>(&value), sizeof(value) });
}

void BinaryStreamWriter::WriteBlob(gsl::span<const unsigned char> data)
{
	ExpandBuffer(data.size_bytes());
	memcpy(&_data[_currentPosition], data.data(), data.size_bytes());

	_currentPosition += data.size_bytes();
}

void BinaryStreamWriter::WriteBlob(const void* data, size_t bytes)
{
    ExpandBuffer(bytes);
    memcpy(&_data[_currentPosition], data, bytes);

    _currentPosition += bytes;
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
    size_t newSize = Max(_data.size(), _currentPosition + newMinSize);
    _data.resize(newSize);
}

bool BinaryStreamWriter::TryWriteFile(const char* path)
{
    std::vector<unsigned char> contents;
    if (!TryReadFileContents(path, contents))
    {
        return false;
    }

    _data.insert(_data.end(), contents.begin(), contents.end());
    _currentPosition += contents.size();

    return true;
}
