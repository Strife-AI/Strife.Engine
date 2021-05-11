#pragma once

#include <cstdio>
#include <vector>
#include <gsl/span>

class Cipher;

class BinaryStreamWriter
{
public:
    void WriteInt(int value);
    void WriteFloat(float value);
    void WriteBlob(gsl::span<const unsigned char> data);
    void WriteBlob(const void* data, size_t bytes);
    bool TryWriteFile(const char* path);
    void Seek(int offset);
    [[nodiscard]] int CurrentPosition() const;
    void WriteToFile(const char* fileName);
    void EncryptRange(int startInclusive, int endExclusive, Cipher& cipher);
    int TotalSize() const { return _data.size(); }

    std::vector<unsigned char>& GetData()
    {
        return _data;
    }

private:
    void ExpandBuffer(int newMinSize);

    std::vector<unsigned char> _data;
    size_t _currentPosition = 0;
};
