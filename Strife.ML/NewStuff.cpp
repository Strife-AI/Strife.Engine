#include "NewStuff.hpp"

namespace StrifeML
{
    void ObjectSerializer::AddBytes(unsigned char* data, int size)
    {
        if (hadError)
        {
            return;
        }

        if (isReading)
        {
            int i;
            for (i = 0; i < size && readOffset < bytes.size(); ++i)
            {
                data[i] = bytes[readOffset++];
            }

            if (i != size)
            {
                // Ran out of bytes
                hadError = true;
            }
        }
        else
        {
            for (int i = 0; i < size; ++i)
            {
                bytes.push_back(data[i]);
            }
        }
    }
}
