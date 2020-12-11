#pragma once

class cOAL_Sample;

class SoundEffect
{
public:
    SoundEffect(cOAL_Sample* sample_)
        : sample(sample_)
    {
        
    }

    void EnableLooping();

    cOAL_Sample* sample;
};