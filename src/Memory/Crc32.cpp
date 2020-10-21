#include "Crc32.hpp"

unsigned int Crc32(const char* str)
{
    unsigned int crc = 0;

    while (*str)
    {
        const unsigned int byte = *str++;
        const unsigned int index = (crc ^ (byte << 24)) >> 24;

        crc = (crc << 8) ^ static_cast<unsigned int>(CrcTable[index]);
    }

    return crc;
}

unsigned int Crc32(const char* data, int size)
{
    unsigned int crc = 0;

    for(int i = 0; i < size; ++i)
    {
        const unsigned int byte = data[i];
        const unsigned int index = (crc ^ (byte << 24)) >> 24;

        crc = (crc << 8) ^ static_cast<unsigned int>(CrcTable[index]);
    }

    return crc;
}

unsigned int Crc32AddByte(unsigned int currentCrc32, unsigned char byte)
{
    const unsigned int index = (currentCrc32 ^ (byte << 24)) >> 24;

    return (currentCrc32 << 8) ^ static_cast<unsigned int>(CrcTable[index]);
}

unsigned int Crc32AddBytes(unsigned int currentCrc32, const char* data, int size)
{
    for (int i = 0; i < size; ++i)
    {
        currentCrc32 = Crc32AddByte(currentCrc32, data[i]);
    }

    return currentCrc32;
}