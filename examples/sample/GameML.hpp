#pragma once

#include "../../Strife.ML/NewStuff.hpp"
#include "Math/Vector2.hpp"

struct PlayerModelInput : StrifeML::ISerializable
{
    void Serialize(StrifeML::ObjectSerializer& serializer) override
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
    void MakeDecision(gsl::span<const InputType> input, OutputType& outOutput) override
    {

    }
};

struct PlayerDecider : StrifeML::Decider<PlayerNetwork>
{
    
};

struct PlayerTrainer : StrifeML::ITrainer<PlayerNetwork>
{
    
};