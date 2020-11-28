#pragma once
#include "../../Strife.ML/NewStuff.hpp"

struct INeuralNetworkEntity
{
    virtual void UpdateNeuralNetwork(float deltaTime) = 0;
    virtual ~INeuralNetworkEntity() = default;
};

template<typename TNeuralNetwork>
struct NeuralNetworkComponent : ComponentTemplate<NeuralNetworkComponent<TNeuralNetwork>>
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    NeuralNetworkComponent(int decisionSequenceLength_ = 1)
        : decisionInputs(StrifeML::MlUtil::MakeSharedArray<InputType>(decisionSequenceLength_)),
        decisionSequenceLength(decisionSequenceLength_)
    {

    }

    virtual ~NeuralNetworkComponent() = default;

    std::function<void(InputType& input)> collectData;

    void Update(float deltaTime) override
    {
        //networkContext->decider->MakeDecision(serializedDecisionInputs, decisionSequenceLength);
    }

    void SetNetwork(const char* name)
    {
        // TODO
    }



    StrifeML::NetworkContext<NetworkType>* networkContext = nullptr;
    // std::shared_ptr<typename IDecider<NetworkType>::MakeDecisionWorkItem> decisionInProgress;
    std::shared_ptr<InputType[]> decisionInputs;
    std::shared_ptr<StrifeML::SerializedModel[]> serializedDecisionInputs;

    int decisionSequenceLength = 1;
};