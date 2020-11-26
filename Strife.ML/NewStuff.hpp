#pragma once
#include <memory>
#include <unordered_set>
#include <gsl/span>



#include "ThreadPool.hpp"
#include "torch/nn/module.h"

struct INeuralNetworkInternal : torch::nn::Module
{
    virtual ~INeuralNetworkInternal() = default;
};

template<typename TInput>
struct ModelInput
{
    ModelInput(const gsl::span<TInput>& inputs_)
        : inputs(inputs_)
    {
        
    }

    const gsl::span<TInput> inputs;
};

template<typename TInput, typename TOutput>
struct INeuralNetwork : INeuralNetworkInternal
{
    using InputType = TInput;
    using OutputType = TOutput;

    virtual void MakeDecision(const ModelInput<InputType>& input, OutputType& outOutput) = 0;
};

struct IDeciderInternal
{
    virtual ~IDeciderInternal() = default;
};

template<typename TNeuralNetwork>
struct IDecider : IDeciderInternal
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    IDecider()
    {
        static_assert(std::is_base_of_v<INeuralNetworkInternal, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
    }

    struct MakeDecisionWorkItem : ThreadPoolWorkItem<OutputType>
    {
        MakeDecisionWorkItem(const std::shared_ptr<TNeuralNetwork>& network_, const std::shared_ptr<InputType[]>& input, int inputSize_)
            : network(network_),
            inputData(input),
            inputSize(inputSize_)
        {
            
        }

        void Execute() override
        {
            OutputType output;
            ModelInput<InputType> input(gsl::span<InputType>(inputData.get(), inputSize));
            network->MakeDecision(input, output);
            _result = output;
        }

        std::shared_ptr<TNeuralNetwork> network;
        std::shared_ptr<InputType[]> inputData;
        int inputSize;
    };

    std::shared_ptr<TNeuralNetwork> network;

    std::shared_ptr<MakeDecisionWorkItem> MakeDecision(const std::shared_ptr<InputType[]>& input, int inputSize)
    {
        auto threadPool = ThreadPool::GetInstance();
        auto workItem = std::make_shared<MakeDecisionWorkItem>(network, input, inputSize);
        threadPool->StartItem(workItem);
        return workItem;
    }
};

struct ITrainerInternal
{
    virtual ~ITrainerInternal() = default;
};

template<typename TNeuralNetwork>
struct ITrainer : ITrainerInternal
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    ITrainer()
    {
        static_assert(std::is_base_of_v<INeuralNetworkInternal, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
    }
};

template<typename TNeuralNetwork>
struct NetworkContext : ITrainerInternal
{
    IDecider<TNeuralNetwork>* decider;
    ITrainer<TNeuralNetwork>* trainer;

    virtual ~NetworkContext() = default;
};

namespace MlUtil
{
    template<typename T>
    std::shared_ptr<T[]> MakeSharedArray(int count)
    {
        // This *should* go away with C++ 17 since it should provide a version of std::make_shared<> for arrays, but that doesn't seem
        // to be the case in MSVC
        return std::shared_ptr<T[]>(new T [count], [](T* ptr)
        {
            delete[] ptr;
        });
    }
}

template<typename TNeuralNetwork>
struct INeuralNetworkEntity
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    INeuralNetworkEntity(int decisionSequenceLength_ = 1)
        : decisionInputs(MlUtil::MakeSharedArray<InputType>(decisionSequenceLength_)),
        decisionSequenceLength(decisionSequenceLength_)
    {
        
    }

    virtual ~INeuralNetworkEntity() = default;

    virtual void CollectData(InputType& outInput) = 0;
    virtual void ReceiveDecision(OutputType& output) = 0;

    void MakeDecision()
    {
        networkContext->decider->MakeDecision(decisionInputs, decisionSequenceLength);
    }

    void SetNetwork(const char* name)
    {
        // TODO
    }

    NetworkContext<NetworkType>* networkContext = nullptr;
    std::shared_ptr<typename IDecider<NetworkType>::MakeDecisionWorkItem> decisionInProgress;
    std::shared_ptr<InputType[]> decisionInputs;
    int decisionSequenceLength = 1;
};

struct NeuralNetworkManager
{
    template<typename TDecider>
    TDecider* CreateDecider()
    {
        static_assert(std::is_base_of_v<IDeciderInternal, TDecider>, "Decider must inherit from IDecider<TInput, TOutput>");
        auto decider = std::make_unique<TDecider>();
        auto deciderPtr = decider.get();
        _deciders.emplace(std::move(decider));
        return deciderPtr;
    }

    template<typename TTrainer>
    TTrainer* CreateTrainer()
    {
        static_assert(std::is_base_of_v<ITrainerInternal, TTrainer>, "Trainer must inherit from ITrainer<TInput, TOutput>");
        auto decider = std::make_unique<TTrainer>();
        auto deciderPtr = decider.get();
        _trainers.emplace(std::move(decider));
        return deciderPtr;
    }

    template<typename TDecider, typename TTrainer>
    void CreateNetwork(const char* name, TDecider* decider, TTrainer* trainer)
    {
        static_assert(std::is_same_v<typename TDecider::NetworkType, typename TTrainer::NetworkType>, "Trainer and decider must accept the same type of neural network");
    }

    template<typename TNeuralNetwork>
    NetworkContext<TNeuralNetwork>* GetNetwork(const char* name)
    {
        return nullptr;
    }

private:
    std::unordered_set<std::unique_ptr<IDeciderInternal>> _deciders;
    std::unordered_set<std::unique_ptr<ITrainerInternal>> _trainers;
};