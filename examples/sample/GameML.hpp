#pragma once

#include "../../Strife.ML/NewStuff.hpp"
#include "Math/Vector2.hpp"

struct PlayerModelInput : StrifeML::IModel
{
    void Serialize(StrifeML::ModelSerializer& serializer) override
    {
        serializer
            .Add(velocity);
    }

    Vector2 velocity;
};

struct PlayerDecision
{
    
};

struct PlayerNetwork : StrifeML::NeuralNetwork<PlayerModelInput, PlayerDecision>
{
    void DoMakeDecision(gsl::span<InputType> input, OutputType& outOutput) override
    {

    }
};

struct PlayerDecider : StrifeML::Decider<PlayerNetwork>
{
    
};

struct PlayerTrainer : StrifeML::ITrainer<PlayerNetwork>
{
    
};