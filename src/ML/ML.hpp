#pragma once
#include "../../Strife.ML/NewStuff.hpp"

struct INeuralNetworkEntity
{
    virtual void Update(float deltaTime) = 0;
    virtual ~INeuralNetworkEntity() = default;
};

template<typename TNeuralNetwork>
struct NeuralNetworkEntity : INeuralNetworkEntity
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    NeuralNetworkEntity(int decisionSequenceLength_ = 1)
        : decisionInputs(StrifeML::MlUtil::MakeSharedArray<InputType>(decisionSequenceLength_)),
        decisionSequenceLength(decisionSequenceLength_)
    {

    }

    virtual ~NeuralNetworkEntity() = default;

    virtual void CollectData(InputType& outInput) = 0;
    virtual void ReceiveDecision(OutputType& output) = 0;

    virtual void Update(float deltaTime) override
    {
        
    }

    void SetNetwork(const char* name)
    {
        // TODO
    }

    StrifeML::NetworkContext<NetworkType>* networkContext = nullptr;
    // std::shared_ptr<typename IDecider<NetworkType>::MakeDecisionWorkItem> decisionInProgress;
    std::shared_ptr<InputType[]> decisionInputs;
    int decisionSequenceLength = 1;
};