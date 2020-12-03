#pragma once
#include <memory>
#include <unordered_set>
#include <gsl/span>

#include "../src/Memory/Grid.hpp"
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

    struct TrainingBatchResult
    {
        float loss = 0;
    };

    template<typename TInput, typename TOutput>
    struct Sample
    {
        TInput input;
        TOutput output;
    };

    template<typename TInput, typename TOutput>
    struct NeuralNetwork : INeuralNetwork
    {
        using InputType = TInput;
        using OutputType = TOutput;
        using SampleType = Sample<InputType, OutputType>;

        virtual void MakeDecision(gsl::span<const TInput> input, TOutput& output) = 0;
        virtual void TrainBatch(Grid<const SampleType> input, TrainingBatchResult& outResult) = 0;
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

    template<typename TSample, typename TSelector>
    class GroupedSampleView
    {
    public:

    private:
        std::function<TSelector(const TSample& sample)> _selector;
    };

    template<typename TSample>
    class SampleSequence
    {
    public:
        bool TryGetSampleBySequenceId(int sequenceId, TSample& outSample)
        {
            if (sequenceId < 0 || sequenceId >= _serializedSamples.size())
            {
                return false;
            }

            auto& serializedSample = _serializedSamples[sequenceId];
            ObjectSerializer serializer(serializedSample.bytes, true);
            outSample.input.Serialize(serializer);
            outSample.output.Serialize(serializer);

            return !serializer.hadError;
        }

        template<typename TSelector>
        void CreateGroupedView(const std::function<TSelector(const TSample& sample)> selector)
        {

        }

    private:
        std::vector<SerializedObject> _serializedSamples;
    };

    template<typename TSample>
    class SampleRepository
    {
    public:
        using Sequence = SampleSequence<TSample>;

        Sequence* CreateSequence(const char* name)
        {
            // TODO check for duplicate
            _sequencesByName[name] = std::make_unique<Sequence>();
            return _sequencesByName[name].get();
        }

    private:
        std::unordered_map<std::string, std::unique_ptr<Sequence>> _sequencesByName;

    };

    struct ITrainerInternal
    {
        virtual ~ITrainerInternal() = default;
    };

    template<typename TNeuralNetwork>
    struct RunTrainingBatchWorkItem : ThreadPoolWorkItem<TrainingBatchResult>
    {
        using InputType = typename TNeuralNetwork::InputType;
        using OutputType = typename TNeuralNetwork::OutputType;
        using NetworkType = TNeuralNetwork;
        using SampleType = Sample<InputType, OutputType>;

        RunTrainingBatchWorkItem(std::shared_ptr<TNeuralNetwork> network_, std::shared_ptr<SampleType[]> samples_, int batchSize_, int sequenceLength_)
            : network(network_),
            samples(samples_),
            batchSize(batchSize_),
            sequenceLength(sequenceLength_)
        {
            
        }

        void Execute() override
        {
            Grid<SampleType> input(samples.get(), batchSize, sequenceLength);
            network->TrainBatch(input, _result);
        }

        std::shared_ptr<TNeuralNetwork> network;
        std::shared_ptr<SampleType[]> samples;
        int batchSize;
        int sequenceLength;
    };

    template<typename TNeuralNetwork>
    struct ITrainer : ITrainerInternal
    {
        using InputType = typename TNeuralNetwork::InputType;
        using OutputType = typename TNeuralNetwork::OutputType;
        using NetworkType = TNeuralNetwork;
        using SampleType = Sample<InputType, OutputType>;

        ITrainer()
        {
            static_assert(std::is_base_of_v<INeuralNetwork, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
        }

        void AddSample(const InputType& input, const OutputType& output)
        {
            sampleLock.Lock();
            ReceiveSample(input, output);
            sampleLock.Unlock();
        }

        std::shared_ptr<RunTrainingBatchWorkItem<TNeuralNetwork>> RunBatch(std::shared_ptr<SampleType[]> sampleStorage, int batchSize, int sequenceLength)
        {
            sampleLock.Lock();
            bool successful = TryCreateBatch(Grid<SampleType>(batchSize, sequenceLength, sampleStorage.get()));
            sampleLock.Unlock();

            if(successful)
            {
                auto threadPool = ThreadPool::GetInstance();
                auto workItem = std::make_shared<RunTrainingBatchWorkItem<TNeuralNetwork>>(sampleStorage, batchSize, sequenceLength);
                threadPool->StartItem(workItem);
                return workItem;
            }
            else
            {
                return nullptr;
            }
        }

        virtual bool TryCreateBatch(Grid<SampleType> outBatch)
        {
            int batchSize = outBatch.Rows();
            for(int i = 0; i < batchSize; ++i)
            {
                if (!TrySelectSequenceSamples(gsl::span<InputType>(&outBatch[i], outBatch.Cols())))
                {
                    return false;
                }
            }

            return true;
        }

        virtual void ReceiveSample(const InputType& input, const OutputType& output) { }

        virtual bool TrySelectSequenceSamples(gsl::span<InputType> outSequence) { return false; }

        SpinLock sampleLock;
        SampleRepository<SampleType> sampleRepository;
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
}