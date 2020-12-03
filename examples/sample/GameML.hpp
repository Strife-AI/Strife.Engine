#pragma once

#include "../../Strife.ML/NewStuff.hpp"
#include "Math/Random.hpp"
#include "Math/Vector2.hpp"
#include "ML/ML.hpp"

struct PlayerModelInput : StrifeML::ISerializable
{
    void Serialize(StrifeML::ObjectSerializer& serializer) override
    {
        serializer
            .Add(velocity)
            .Add(grid);
    }

    Vector2 velocity;
    GridSensorOutput<40, 40> grid;
};

struct PlayerDecision
{
    Vector2 velocity;
};

struct PlayerNetwork : StrifeML::NeuralNetwork<PlayerModelInput, PlayerDecision>
{
    void MakeDecision(gsl::span<const InputType> input, OutputType& output) override
    {
        //output.velocity = Rand({ -1, 1 }, { -1, 1 }).Normalize() * 200;
    }

    void TrainBatch(Grid<const StrifeML::Sample<PlayerModelInput, PlayerDecision>> input, StrifeML::TrainingBatchResult& outResult) override
    {
        
    }
};

struct PlayerDecider : StrifeML::Decider<PlayerNetwork>
{
    
};

struct PlayerTrainer : StrifeML::ITrainer<PlayerNetwork>
{
    
};