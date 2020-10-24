#pragma once

enum class ObservedObject
{
    Nothing,
    Wall,
    Breakable,
    GravityWell,
    JumpPad,
    Gate,
    Teleporter,
    Elevator,
    ConveyorForward,
    ConveyorBackward,
    Door,

    TotalObjects // use this to refer to the total number of values in this enum
};
