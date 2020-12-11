#include "SoundEffect.hpp"

#include "OALWrapper/OAL_Funcs.h"

void SoundEffect::EnableLooping()
{
    OAL_Sample_SetLoop(sample, true);
}
