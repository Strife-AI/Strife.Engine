#pragma once

#include <cstdio>
#include <vector>

class Cipher;

class BinaryStreamWriter
{
public:
    void WriteInt(int value);
    void WriteFloat(float value);
    void WriteBlob(const void* data, int size);
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
    int _currentPosition = 0;
};
