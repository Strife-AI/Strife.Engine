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

struct PlayerNetwork : StrifeML::NeuralNetwork<PlayerModelInput, PlayerDecision>
{
    
};

struct PlayerDecider : StrifeML::IDecider<PlayerNetwork>
{
    
};

struct PlayerTrainer : StrifeML::ITrainer<PlayerNetwork>
{
    
};