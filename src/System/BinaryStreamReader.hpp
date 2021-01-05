#pragma once

#include <cstdio>
#include <gsl/span>

class Cipher;

class BinaryStreamReader
{
public:
    void Open(const char* fileName);
    void Open(const unsigned char* buffer, int size);
    void Open(gsl::span<std::byte> data);

    int ReadInt();
    float ReadFloat();
    void ReadBlob(void* dest, int size);
    void Seek(int offset);
    void Close();
    int TotalSize() const { return _bufferSize; }

    void DecryptRange(int startInclusive, int endExclusive, Cipher& cipher);

    ~BinaryStreamReader();;

private:
    bool _freeOnClose = false;
    const unsigned char* _buffer = nullptr;
    int _bufferSize = 0;
    int _bufferIndex = 0;
};
