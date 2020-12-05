#pragma once

#include "../../Strife.ML/NewStuff.hpp"
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

enum class PlayerAction
{
    Nothing,
    Up,
    Down,
    Left,
    Right
};

struct PlayerDecision : StrifeML::ISerializable
{
    void Serialize(StrifeML::ObjectSerializer& serializer) override
    {
        serializer
            .Add(velocity)
            .Add(action);
    }

    Vector2 velocity;
    PlayerAction action;
};

struct PlayerNetwork : StrifeML::NeuralNetwork<PlayerModelInput, PlayerDecision, 1>
{
    void MakeDecision(gsl::span<const InputType> input, OutputType& output) override
    {

    }

    void TrainBatch(Grid<const SampleType> input, StrifeML::TrainingBatchResult& outResult) override
    {
        
    }
};

struct PlayerDecider : StrifeML::Decider<PlayerNetwork>
{
    
};

struct PlayerTrainer : StrifeML::ITrainer<PlayerNetwork>
{
    PlayerTrainer()
        : ITrainer<PlayerNetwork>(32, 1)
    {
        samples = sampleRepository.CreateSampleSet("player-samples");
        samplesByActionType = samples
            ->CreateGroupedView<PlayerAction>()
            ->GroupBy([=](const SampleType& sample) { return sample.output.action; });
    }

    void ReceiveSample(const SampleType& sample) override
    {
        samples->AddSample(sample);
    }

    bool TrySelectSequenceSamples(gsl::span<SampleType> outSequence) override
    {
        return samplesByActionType->TryPickRandomSequence(outSequence);
    }

    StrifeML::SampleSet<SampleType>* samples;
    StrifeML::GroupedSampleView<SampleType, PlayerAction>* samplesByActionType;
};