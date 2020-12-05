#pragma once
#include <memory>
#include <random>
#include <unordered_set>
#include <gsl/span>

#include "Container/Grid.hpp"
#include "Thread/TaskScheduler.hpp"
#include "Thread/ThreadPool.hpp"
#include "torch/nn/module.h"
#include "torch/serialize.h"

namespace StrifeML
{
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
        template<typename T, std::enable_if_t<!(std::is_arithmetic_v<T> || std::is_enum_v<T>)>* = nullptr>
        ObjectSerializer& Add(T& value)
        {
            Serialize(value, *this);
            return *this;
        }

        // Arithmetic types are default serialized to bytes; everything else needs to define a custom serialization method
        template<typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>* = nullptr>
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

    struct RandomNumberGenerator
    {
        RandomNumberGenerator()
            : _rng(std::random_device()())
        {
            
        }

        int RandInt(int min, int max)
        {
            return std::uniform_int_distribution<int>(min, max)(_rng);
        }

        int RandFloat(float min, float max)
        {
            return std::uniform_real_distribution<float>(min, max)(_rng);
        }

    private:
        std::mt19937 _rng;
    };

    template<typename TSample>
    class SampleSet;

    template<typename TSample>
    struct IGroupedSampleView
    {
        virtual ~IGroupedSampleView() = default;

        virtual void AddSample(const TSample& sample, int sampleId) = 0;
    };

    template<typename TSample, typename TSelector>
    class GroupedSampleView : public IGroupedSampleView<TSample>
    {
    public:
        GroupedSampleView(SampleSet<TSample>* owner)
            : _owner(owner)
        {
            
        }

        GroupedSampleView* GroupBy(std::function<TSelector(const TSample& sample)> selector)
        {
            _selector = selector;
            return this;
        }

        bool TryPickRandomSequence(gsl::span<TSample> outSamples);

        void AddSample(const TSample& sample, int sampleId) override
        {
            if(_selector == nullptr)
            {
                return;
            }

            _samplesBySelectorType[_selector(sample)].push_back(sampleId);
        }

    private:
        SampleSet<TSample>* _owner;
        std::function<TSelector(const TSample& sample)> _selector;
        std::unordered_map<TSelector, std::vector<int>> _samplesBySelectorType;
        std::vector<const std::vector<int>*> _validSampleGroups;
    };

    template<typename TSample>
    class SampleSet
    {
    public:
        SampleSet(RandomNumberGenerator& rng)
            : _rng(rng)
        {
            
        }

        bool TryGetSampleById(int sampleId, TSample& outSample)
        {
            if (sampleId < 0 || sampleId >= _serializedSamples.size())
            {
                return false;
            }

            auto& serializedSample = _serializedSamples[sampleId];
            ObjectSerializer serializer(serializedSample.bytes, true);
            outSample.input.Serialize(serializer);
            outSample.output.Serialize(serializer);

            return !serializer.hadError;
        }

        int AddSample(const TSample& sample)
        {
            _serializedSamples.emplace_back();
            int sampleId = _serializedSamples.size() - 1;
            auto& serializedObject = _serializedSamples[sampleId];
            ObjectSerializer serializer(serializedObject.bytes, false);

            // This is safe because the serializer is in reading mode
            auto& mutableSample = const_cast<TSample&>(sample);

            mutableSample.input.Serialize(serializer);
            mutableSample.output.Serialize(serializer);

            for(auto& group : _groupedSamplesViews)
            {
                group->AddSample(sample, sampleId);
            }

            return sampleId;
        }

        template<typename TSelector>
        GroupedSampleView<TSample, TSelector>* CreateGroupedView()
        {
            auto group = std::make_unique<GroupedSampleView<TSample, TSelector>>(this);
            auto ptr = group.get();
            _groupedSamplesViews.emplace_back(std::move(group));
            return ptr;
        }

        RandomNumberGenerator& GetRandomNumberGenerator() const
        {
            return _rng;
        }

    private:
        std::vector<SerializedObject> _serializedSamples;
        std::vector<std::unique_ptr<IGroupedSampleView<TSample>>> _groupedSamplesViews;
        RandomNumberGenerator& _rng;
    };

    template <typename TSample, typename TSelector>
    bool GroupedSampleView<TSample, TSelector>::TryPickRandomSequence(gsl::span<TSample> outSamples)
    {
        int minSampleId = outSamples.size() - 1;
        _validSampleGroups.clear();

        for (const auto& groupPair : _samplesBySelectorType)
        {
            const auto& group = groupPair.second;
            if (group.empty())
            {
                continue;
            }

            // The largest sample id in this group isn't big enough to have N samples before it
            if (group[group.size() - 1] < minSampleId)
            {
                continue;
            }

            _validSampleGroups.push_back(&group);
        }

        if (_validSampleGroups.empty())
        {
            return false;
        }

        auto& rng = _owner->GetRandomNumberGenerator();
        int randomSelector = rng.RandInt(0, _validSampleGroups.size() - 1);
        auto& groupToSampleFrom = *_validSampleGroups[randomSelector];
        int groupIndexStart = 0;
        int endSampleId = 0;
        bool found = false;

        while(groupIndexStart < groupToSampleFrom.size())
        {
            int groupIndex = rng.RandInt(groupIndexStart, groupToSampleFrom.size() - 1);
            if(groupToSampleFrom[groupIndex] < minSampleId)
            {
                groupIndexStart = groupIndex + 1;
            }
            else
            {
                endSampleId = groupIndex;
                found = true;
                break;
            }
        }

        // Should be impossible because we checked to make sure the list had at least one sample id big enough
        assert(found);

        for(int i = 0; i < outSamples.size(); ++i)
        {
            _owner->TryGetSampleById(endSampleId - (outSamples.size() - 1 - i), outSamples[i]);
        }

        return true;
    }

    template<typename TSample>
    class SampleRepository
    {
    public:
        SampleRepository(RandomNumberGenerator& rng)
            : _rng(rng)
        {
            
        }

        SampleSet<TSample>* CreateSampleSet(const char* name)
        {
            // TODO check for duplicate
            _sequencesByName[name] = std::make_unique<SampleSet<TSample>>(_rng);
            return _sequencesByName[name].get();
        }

    private:
        std::unordered_map<std::string, std::unique_ptr<SampleSet<TSample>>> _sequencesByName;
        RandomNumberGenerator& _rng;
    };

    struct ITrainerInternal
    {
        virtual ~ITrainerInternal() = default;
    };

    template<typename TNeuralNetwork>
    struct ITrainer;

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

        void Execute() override;

        std::shared_ptr<TNeuralNetwork> network;
        std::shared_ptr<SampleType[]> samples;
        std::shared_ptr<ITrainer<TNeuralNetwork>> trainer;
        int batchSize;
        int sequenceLength;
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

        void SetNewNetwork(std::stringstream& stream)
        {
            newNetworkLock.Lock();
            newNetwork = std::move(stream);
            newNetworkLock.Lock();
        }

        bool TryGetNewNetwork(std::stringstream& outNewNetwork)
        {
            newNetworkLock.Lock();
            bool hasNewNetwork = newNetwork.has_value();
            outNewNetwork = std::move(*newNetwork);
            newNetwork = std::nullopt;
            newNetworkLock.Unlock();
            return hasNewNetwork;
        }

        Decider<TNeuralNetwork>* decider;
        ITrainer<TNeuralNetwork>* trainer;

        std::optional<std::stringstream> newNetwork;
        SpinLock newNetworkLock;

        virtual ~NetworkContext() = default;
    };

    template<typename TNeuralNetwork>
    struct ITrainer : ITrainerInternal, std::enable_shared_from_this<ITrainer<TNeuralNetwork>>
    {
        using InputType = typename TNeuralNetwork::InputType;
        using OutputType = typename TNeuralNetwork::OutputType;
        using NetworkType = TNeuralNetwork;
        using SampleType = Sample<InputType, OutputType>;

        ITrainer()
            : sampleRepository(rng)
        {
            static_assert(std::is_base_of_v<INeuralNetwork, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
        }

        void AddSample(const InputType& input, const OutputType& output)
        {
            sampleLock.Lock();
            ReceiveSample(input, output);
            sampleLock.Unlock();
        }

        std::shared_ptr<RunTrainingBatchWorkItem<TNeuralNetwork>> RunBatch()
        {
            sampleLock.Lock();
            bool successful = TryCreateBatch(Grid<SampleType>(batchSize, sequenceLength, trainingInput.get()));
            sampleLock.Unlock();

            if(successful)
            {
                auto threadPool = ThreadPool::GetInstance();
                auto workItem = std::make_shared<RunTrainingBatchWorkItem<TNeuralNetwork>>(trainingInput, batchSize, sequenceLength);
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
                if (!TrySelectSequenceSamples(gsl::span<SampleType>(outBatch[i], outBatch.Cols())))
                {
                    return false;
                }
            }

            return true;
        }

        void NotifyTrainingComplete(std::stringstream& serializedNetwork)
        {
            networkContext->SetNewNetwork(serializedNetwork);
        }

        virtual void ReceiveSample(const SampleType& sample) { }

        virtual bool TrySelectSequenceSamples(gsl::span<SampleType> outSequence) { return false; }

        SpinLock sampleLock;
        RandomNumberGenerator rng;
        SampleRepository<SampleType> sampleRepository;
        std::shared_ptr<SampleType[]> trainingInput;
        int batchSize;
        int sequenceLength;
        std::shared_ptr<ScheduledTask> trainTask;
        std::shared_ptr<NetworkContext<TNeuralNetwork>> networkContext;
        bool isRunning = false;
    };

    template<typename TNeuralNetwork>
    void RunTrainingBatchWorkItem<TNeuralNetwork>::Execute()
    {
        Grid<SampleType> input(batchSize, sequenceLength, samples.get());
        //network->TrainBatch(input, _result);
        std::stringstream stream;
        torch::save(network, stream);
        trainer->NotifyTrainingComplete();
    }

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