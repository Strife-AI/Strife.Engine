#pragma once
#include <memory>
#include <unordered_set>

#include "torch/nn/module.h"

struct INeuralNetworkInternal : torch::nn::Module
{
    virtual ~INeuralNetworkInternal() = default;
};

template<typename TInput, typename TOutput>
struct INeuralNetwork : INeuralNetworkInternal
{
    using InputType = TInput;
    using OutputType = TOutput;
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

    virtual void MakeDecision(const InputType& input, OutputType& outDecision) = 0;
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

template<typename TNeuralNetwork>
struct INeuralNetworkEntity
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;

    virtual ~INeuralNetworkEntity() = default;

    virtual void CollectData(InputType& outInput) = 0;
    virtual void ReceiveDecision(OutputType& output) = 0;

    void SetNetwork(const char* name)
    {
        // TODO
    }

    NetworkContext<NetworkType>* networkContext;
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