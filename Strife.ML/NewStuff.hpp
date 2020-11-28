#pragma once
#include <memory>
#include <unordered_set>
#include <gsl/span>
#include "ThreadPool.hpp"
#include "torch/nn/module.h"

namespace StrifeML
{

    struct ModelSerializer
    {
        ModelSerializer(bool isReading) { }

        template<typename T>
        ModelSerializer& Add(T& value) { return *this; }    // TODO
    };

    struct SerializedModel
    {
        template<typename T>
        T Deserialize() { return T(); }
    };

    struct IModel
    {
        virtual ~IModel() = default;

        virtual void Serialize(ModelSerializer& serializer) = 0;
    };

    struct INeuralNetwork : torch::nn::Module
    {
        virtual void MakeDecision(gsl::span<SerializedModel> models, SerializedModel& outModel) { }

        virtual ~INeuralNetwork() = default;
    };

    template<typename TInput, typename TOutput>
    struct NeuralNetwork : INeuralNetwork
    {
        using InputType = TInput;
        using OutputType = TOutput;

        void MakeDecision(gsl::span<SerializedModel> models, SerializedModel& outModel) override
        {
            auto input = std::make_unique<InputType[]>(models.size());
            for (int i = 0; i < models.size(); ++i)
            {
                input[i] = models[i].Deserialize<TInput>();
            }

            OutputType output;
            DoMakeDecision(gsl::span<InputType>(input.get(), models.size()), output);
        }

        virtual void DoMakeDecision(gsl::span<InputType> input, OutputType& outOutput) = 0;
    };

    struct IDecider
    {
        virtual ~IDecider() = default;
    };

    struct MakeDecisionWorkItem : ThreadPoolWorkItem<SerializedModel>
    {
        MakeDecisionWorkItem(std::shared_ptr<INeuralNetwork> network_, std::shared_ptr<SerializedModel[]> input_, int inputLength_);

        void Execute() override;

        std::shared_ptr<INeuralNetwork> network;
        std::shared_ptr<SerializedModel[]> input;
        int inputLength;
    };

    std::shared_ptr<MakeDecisionWorkItem> ScheduleDecision(
        std::shared_ptr<INeuralNetwork> network,
        std::shared_ptr<SerializedModel[]> input,
        int inputLength);

    template<typename TNeuralNetwork>
    struct Decider : IDecider
    {
        using InputType = typename TNeuralNetwork::InputType;
        using OutputType = typename TNeuralNetwork::OutputType;
        using NetworkType = TNeuralNetwork;

        Decider()
        {
            static_assert(std::is_base_of_v<INeuralNetwork, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
        }

        auto MakeDecision(std::shared_ptr<SerializedModel[]> input, int inputLength)
        {
            return ScheduleDecision(network, input, inputLength);
        }

        std::shared_ptr<TNeuralNetwork> network;
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
            static_assert(std::is_base_of_v<INeuralNetwork, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
        }
    };

    struct INetworkContext
    {
        virtual ~INetworkContext() = default;
    };

    template<typename TNeuralNetwork>
    struct NetworkContext : INetworkContext
    {
        NetworkContext(Decider<TNeuralNetwork>* decider_, ITrainer<TNeuralNetwork>* trainer_)
            : decider(decider_),
            trainer(trainer_)
        {
            
        }

        Decider<TNeuralNetwork>* decider;
        ITrainer<TNeuralNetwork>* trainer;

        virtual ~NetworkContext() = default;
    };

    struct StrifeException : std::exception
    {
        StrifeException(const std::string& message_)
            : message(message_)
        {
            
        }

        StrifeException(const char* message_)
            : message(message_)
        {
            
        }

        const char* what() const override
        {
            return message.c_str();
        }

        std::string message;
    };

    namespace MlUtil
    {
        template<typename T>
        std::shared_ptr<T[]> MakeSharedArray(int count)
        {
            // This *should* go away with C++ 17 since it should provide a version of std::make_shared<> for arrays, but that doesn't seem
            // to be the case in MSVC
            return std::shared_ptr<T[]>(new T[count], [](T* ptr)
            {
                delete[] ptr;
            });
        }
    }

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

            auto context = _networksByName.find(name);
            if (context != _networksByName.end())
            {
                throw StrifeException("Network already exists: " + std::string(name));
            }

            _networksByName[name] = std::make_shared < NetworkContext<typename TDecider::NetworkType>(decider, trainer);
        }

        // TODO remove network

        template<typename TNeuralNetwork>
        NetworkContext<TNeuralNetwork>* GetNetwork(const char* name)
        {
            // TODO error handling if missing
            return _networksByName[name];
        }

    private:
        std::unordered_set<std::unique_ptr<IDecider>> _deciders;
        std::unordered_set<std::unique_ptr<ITrainerInternal>> _trainers;
        std::unordered_map<std::string, std::shared_ptr<INetworkContext>> _networksByName;
    };

}