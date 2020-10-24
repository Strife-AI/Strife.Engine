#pragma once

enum class CharacterAction
{
    DoNotRecord = -1,
    Nothing = 0,
    Jump,
    Roll,
    StartSlide,
    StandUp,
    SwimUp,
    SwimDown,

    TotalActions
};