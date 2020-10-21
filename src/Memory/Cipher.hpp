#pragma once
#include <vector>

#include "Math/BitMath.hpp"

class Cipher
{
public:
    Cipher(const std::vector<byte>& addTable, const std::vector<byte>& xorTable, unsigned int extraXor)
        : _addTable(addTable),
        _xorTable(xorTable),
        _extraXor(extraXor)
    {
        
    }

    void Encrypt(byte* begin, byte* end);
    void Decrypt(byte* begin, byte* end);

private:
    static constexpr int InitialCrc32 = 0xF3C2E9AC;

    std::vector<byte> _addTable;
    std::vector<byte> _xorTable;
    unsigned int _extraXor;
};

Cipher GetDefaultCipher();

