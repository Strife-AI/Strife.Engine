#pragma once
#include "OALWrapper/OAL_Stream.h"

class MusicTrack
{
public:
    explicit MusicTrack(cOAL_Stream* stream_)
        : stream(stream_)
    {
        
    }

    cOAL_Stream* stream;
};
