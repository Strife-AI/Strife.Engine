#pragma once

#include <cstdio>
#include <gsl/span>

class Cipher;

class BinaryStreamReader
{
public:
    void Open(const char* fileName);
    void Open(const unsigned char* buffer, size_t size);
    void Open(gsl::span<std::byte> data);

    int ReadInt();
    float ReadFloat();
    void ReadBlob(void* dest, int size);
    void Seek(int offset);
    void Close();
    size_t TotalSize() const { return _bufferSize; }
    const unsigned char* Data() { return _buffer; }

    void DecryptRange(int startInclusive, int endExclusive, Cipher& cipher);

    ~BinaryStreamReader();;

private:
    bool _freeOnClose = false;
    const unsigned char* _buffer = nullptr;
    size_t _bufferSize = 0;
    size_t _bufferIndex = 0;
};
