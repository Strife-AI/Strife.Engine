#pragma once

#include "../../Strife.ML/NewStuff.hpp"
#include "Math/Vector2.hpp"

struct PlayerModelInput
{
    Vector2 velocity;
};

struct PlayerDecision
{
    
};

struct PlayerNetwork : INeuralNetwork<PlayerModelInput, PlayerDecision>
{
    
};

struct PlayerDecider : IDecider<PlayerNetwork>
{
    
};

struct PlayerTrainer : ITrainer<PlayerNetwork>
{
    
};