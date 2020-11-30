#pragma once
#include "Engine.hpp"
#include "../../Strife.ML/NewStuff.hpp"

template<>
inline void StrifeML::Serialize<Vector2>(Vector2& value, StrifeML::ObjectSerializer& serializer)
{
    serializer.Add(value.x).Add(value.y);
}

template<typename TNeuralNetwork>
struct NeuralNetworkComponent : ComponentTemplate<NeuralNetworkComponent<TNeuralNetwork>>
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    NeuralNetworkComponent(int decisionSequenceLength_ = 1, float decisionsPerSecond_ = 1)
        : inputs(StrifeML::MlUtil::MakeSharedArray<InputType>(decisionSequenceLength_)),
        decisionSequenceLength(decisionSequenceLength_),
        decisionsPerSecond(decisionsPerSecond_)
    {

    }

    virtual ~NeuralNetworkComponent() = default;

    void Update(float deltaTime) override;

    void SetNetwork(const char* name)
    {
        auto nnManager = Engine::GetInstance()->GetNeuralNetworkManager();
        networkContext = nnManager->GetNetwork<TNeuralNetwork>(name);
    }

    void MakeDecision();

    StrifeML::NetworkContext<NetworkType>* networkContext = nullptr;
    std::shared_ptr<StrifeML::MakeDecisionWorkItem<TNeuralNetwork>> decisionInProgress;
    std::shared_ptr<InputType[]> inputs;

    std::function<void(InputType& input)> collectData;
    std::function<void(OutputType& decision)> receiveDecision;

    int decisionSequenceLength;
    float decisionsPerSecond;
    float makeDecisionsTimer = 0;
};

template <typename TNeuralNetwork>
void NeuralNetworkComponent<TNeuralNetwork>::Update(float deltaTime)
{
    makeDecisionsTimer -= deltaTime;

    if (makeDecisionsTimer <= 0)
    {
        makeDecisionsTimer = 0;
        MakeDecision();
    }

    OutputType output;
    if(decisionInProgress != nullptr && decisionInProgress->TryGetResult(output))
    {
        if(receiveDecision != nullptr)
        {
            receiveDecision(output);
        }

        decisionInProgress = nullptr;
    }
}

template <typename TNeuralNetwork>
void NeuralNetworkComponent<TNeuralNetwork>::MakeDecision()
{
    // Don't allow making more than one decision at a time
    if (decisionInProgress != nullptr
        && !decisionInProgress->IsComplete())
    {
        return;
    }

    if (collectData == nullptr
        || networkContext == nullptr
        || networkContext->decider == nullptr)
    {
        return;
    }

    // Expire oldest input
    for (int i = 0; i < decisionSequenceLength - 1; ++i)
    {
        inputs[i] = std::move(inputs[i + 1]);
    }

    // Collect new input
    collectData(inputs[decisionSequenceLength - 1]);

    // Start making decision
    decisionInProgress = networkContext->decider->MakeDecision(inputs, decisionSequenceLength);
}
