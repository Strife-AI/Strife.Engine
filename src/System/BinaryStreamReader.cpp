#include <cstdio>
#include <cstring>

#include "BinaryStreamReader.hpp"
#include "FileSystem.hpp"
#include "Memory/Cipher.hpp"

void BinaryStreamReader::Open(const char* fileName)
{
	auto file = OpenFile(fileName, "rb");
	_freeOnClose = true;

	if (file == nullptr)
	{
		FatalError("Failed to open %s for reading", fileName);
	}

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	auto buf = new unsigned char[size];
	int countRead = (int)fread(buf, 1, size, file);
	fclose(file);

	if (countRead != size)
	{
		FatalError("Failed to read file %s contents", fileName);
	}

	_buffer = buf;
	_bufferSize = size;
	_bufferIndex = 0;
}

void BinaryStreamReader::Open(const unsigned char* buffer, int size)
{
	_freeOnClose = false;
	_bufferSize = size;
	_bufferIndex = 0;
	_buffer = buffer;
}

void BinaryStreamReader::Open(gsl::span<std::byte> data)
{
	Open(reinterpret_cast<unsigned char*>(&data[0]), data.size());
}

int BinaryStreamReader::ReadInt()
{
	int value;
	ReadBlob(&value, sizeof(int));

	return value;
}

float BinaryStreamReader::ReadFloat()
{
    float value;
    ReadBlob(&value, sizeof(value));

    return value;
}

void BinaryStreamReader::ReadBlob(void* dest, int size)
{
	if (_bufferIndex + size > _bufferSize)
	{
		FatalError("Reached end of file when reading blob");
	}

	memcpy(dest, _buffer + _bufferIndex, size);
	_bufferIndex += size;
}

void BinaryStreamReader::Seek(int offset)
{
	if (offset < 0 || offset >= _bufferSize != 0)
	{
		FatalError("Read seek failed");
	}

	_bufferIndex = offset;
}

void BinaryStreamReader::Close()
{
	if (_freeOnClose)
	{
		delete [] _buffer;
	}

	_bufferIndex = 0;
	_bufferSize = 0;
	_buffer = nullptr;
}

void BinaryStreamReader::DecryptRange(int startInclusive, int endExclusive, Cipher& cipher)
{
	cipher.Decrypt((byte*)_buffer + startInclusive, (byte*)_buffer + endExclusive);
}

BinaryStreamReader::~BinaryStreamReader()
{
	Close();
}
