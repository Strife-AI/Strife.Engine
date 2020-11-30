#pragma once
#include <memory>
#include <unordered_set>
#include <gsl/span>
#include "Thread/ThreadPool.hpp"
#include "torch/nn/module.h"

namespace StrifeML
{
    struct ISerializable;
    struct ObjectSerializer;

    template<typename T>
    void Serialize(T& value, ObjectSerializer& serializer);

    struct ObjectSerializer
    {
        ObjectSerializer(std::vector<unsigned char>& bytes_, bool isReading_)
            : bytes(bytes_),
            isReading(isReading_)
        {
            
        }

        // Create a template specialization of Serialize<> to serialize custom types
        template<typename T, std::enable_if_t<!std::is_arithmetic_v<T>>* = nullptr>
        ObjectSerializer& Add(T& value)
        {
            Serialize(value, *this);
            return *this;
        }

        // Arithmetic types are default serialized to bytes; everything else needs to define a custom serialization method
        template<typename T, std::enable_if_t<std::is_arithmetic_v<T>>* = nullptr>
        ObjectSerializer& Add(T& value)
        {
            AddBytes(reinterpret_cast<unsigned char*>(&value), sizeof(T));
            return *this;
        }

        void AddBytes(unsigned char* data, int size);

        std::vector<unsigned char>& bytes;
        bool isReading;
        int readOffset = 0;
        bool hadError = false;
    };

    template<typename T>
    void Serialize(T& value, ObjectSerializer& serializer);

    struct SerializedObject
    {
        template<typename T>
        void Deserialize(T& outResult);

        std::vector<unsigned char> bytes;
    };

    template <typename T>
    void SerializedObject::Deserialize(T& outResult)
    {
        static_assert(std::is_base_of_v<ISerializable, T>, "Deserialized type must implement ISerializable");
        ObjectSerializer serializer(bytes, true);
        outResult.Serialize(serializer);

        // TODO assert all bytes are used?
        // TODO check if hadError flag was set
    }

    struct ISerializable
    {
        virtual ~ISerializable() = default;

        virtual void Serialize(ObjectSerializer& serializer) = 0;
    };

    struct INeuralNetwork : torch::nn::Module
    {
        virtual ~INeuralNetwork() = default;
    };

    template<typename TInput, typename TOutput>
    struct NeuralNetwork : INeuralNetwork
    {
        using InputType = TInput;
        using OutputType = TOutput;

        virtual void MakeDecision(gsl::span<const TInput> input, TOutput& output) = 0;
    };

    struct IDecider
    {
        virtual ~IDecider() = default;
    };

    template<typename TNetwork>
    struct MakeDecisionWorkItem : ThreadPoolWorkItem<typename TNetwork::OutputType>
    {
        using InputType = typename TNetwork::InputType;

        MakeDecisionWorkItem(std::shared_ptr<TNetwork> network_, std::shared_ptr<InputType[]> input_, int inputLength_)
            : network(network_),
            input(input_),
            inputLength(inputLength_)
        {
            
        }

        void Execute() override
        {
            network->MakeDecision(gsl::span<InputType>(input.get(), inputLength), _result);
        }

        std::shared_ptr<TNetwork> network;
        std::shared_ptr<InputType[]> input;
        int inputLength;
    };

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

        auto MakeDecision(std::shared_ptr<InputType[]> input, int inputLength)
        {
            auto workItem = std::make_shared<MakeDecisionWorkItem<TNeuralNetwork>>(network, input, inputLength);
            auto threadPool = ThreadPool::GetInstance();
            threadPool->StartItem(workItem);
            return workItem;
        }

        std::shared_ptr<TNeuralNetwork> network = std::make_shared<TNeuralNetwork>();
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
            static_assert(std::is_base_of_v<IDecider, TDecider>, "Decider must inherit from IDecider<TInput, TOutput>");
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

            _networksByName[name] = std::make_shared<NetworkContext<typename TDecider::NetworkType>>(decider, trainer);
        }

        // TODO remove network method

        template<typename TNeuralNetwork>
        NetworkContext<TNeuralNetwork>* GetNetwork(const char* name)
        {
            // TODO error handling if missing or wrong type
            return dynamic_cast<NetworkContext<TNeuralNetwork>*>(_networksByName[name].get());
        }

    private:
        std::unordered_set<std::unique_ptr<IDecider>> _deciders;
        std::unordered_set<std::unique_ptr<ITrainerInternal>> _trainers;
        std::unordered_map<std::string, std::shared_ptr<INetworkContext>> _networksByName;
    };

}