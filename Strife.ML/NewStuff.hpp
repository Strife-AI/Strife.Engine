#pragma once
#include <memory>
#include <random>
#include <unordered_set>
#include <gsl/span>
#include <cstdarg>
#include <cassert>

#include "Container/Grid.hpp"
#include "Thread/TaskScheduler.hpp"
#include "Thread/ThreadPool.hpp"

namespace torch
{
    namespace nn
    {
        class Module;
    }
}

namespace StrifeML
{
    std::shared_ptr<torch::nn::Module> CreateModule();
    void TorchLoad(std::shared_ptr<torch::nn::Module> module, std::stringstream& stream);
    void TorchSave(std::shared_ptr<torch::nn::Module> module, std::stringstream& stream);

    namespace MlUtil
    {
        template<typename T>
        struct SharedArray
        {
            SharedArray(int count_)
                : count(count_)
            {
                data = MakeSharedArray(count);
            }

            std::shared_ptr<T> data;
            int count;

        private:
            std::shared_ptr<T> MakeSharedArray(int count)
            {
                // This *should* go away with C++ 17 since it should provide a version of std::make_shared<> for arrays, but that doesn't seem
                // to be the case in MSVC
                return std::shared_ptr<T>(new T[count], [](T* ptr)
                {
                    delete[] ptr;
                });
            }
        };
    }

    struct StrifeException : std::exception
    {
        StrifeException(const std::string& message_)
            : message(message_)
        {

        }

        StrifeException(const char* format, ...)
        {
            va_list args;
            va_start (args, format);

            char buf[1024];
            vsnprintf(buf, sizeof(buf), format, args);
            message = buf;
        }

        const char* what()  const noexcept override
        {
            return message.c_str();
        }

        std::string message;
    };

    struct ISerializable;
    struct ObjectSerializer;

    template<typename T>
    const char* ObjectSerializerName();

    template<> inline const char* ObjectSerializerName<float>() { return "float"; }
    template<> inline const char* ObjectSerializerName<int>() { return "int"; }

    struct ObjectSerializerProperty
    {
        ObjectSerializerProperty()
            : type(nullptr),
            offset(0)
        {

        }

        ObjectSerializerProperty(const char* type, int offset)
            : type(type),
            offset(offset)
        {

        }

        const char* type;
        int offset;
    };

    struct ObjectSerializerSchema
    {
        template<typename T>
        void AddProperty(const char* name, int offset)
        {
            propertiesByName[name] = ObjectSerializerProperty(ObjectSerializerName<T>(), offset);
        }

        std::unordered_map<std::string, ObjectSerializerProperty> propertiesByName;
    };

    template<typename T, typename Enable = void>
    struct Serializer;

    struct ObjectSerializer
    {
        ObjectSerializer(std::vector<unsigned char>& bytes_, bool isReading_, ObjectSerializerSchema* schema = nullptr)
            : bytes(bytes_),
              isReading(isReading_),
              schema(schema)
        {

        }

        template<typename T>
        ObjectSerializer& Add(T& value, const char* name)
        {
            if (schema != nullptr)
            {
                schema->template AddProperty<T>(name, (int)bytes.size());
            }

            Serializer<T>::Serialize(value, *this);
            return *this;
        }

        void AddBytes(unsigned char* data, int size);

        template<typename T>
        void AddBytes(T* data, int count)
        {
            AddBytes(reinterpret_cast<unsigned char*>(data), count * sizeof(T));
        }

        void Seek(int offset)
        {
            if (offset < 0 || offset >= bytes.size())
            {
                throw StrifeException("Invalid read offset");
            }

            readOffset = offset;
        }

        std::vector<unsigned char>& bytes;
        ObjectSerializerSchema* schema = nullptr;

        bool isReading;
        int readOffset = 0;
        bool hadError = false;
    };

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

    struct INeuralNetwork
    {
        INeuralNetwork()
            : module(CreateModule())
        {

        }

        std::shared_ptr<torch::nn::Module> module;
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

    template<typename TInput, typename TOutput, int SeqLength>
    struct NeuralNetwork : INeuralNetwork
    {
        using InputType = TInput;
        using OutputType = TOutput;
        using SampleType = Sample<InputType, OutputType>;
        static constexpr int SequenceLength = SeqLength;

        virtual void MakeDecision(Grid<const TInput> input, TOutput& output) = 0;
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

        MakeDecisionWorkItem(std::shared_ptr<TNetwork> network_, MlUtil::SharedArray<InputType> input_, int inputLength_)
            : network(network_),
              input(input_),
              inputLength(inputLength_)
        {

        }

        void Execute() override
        {
            network->MakeDecision(Grid<const InputType>(1, inputLength, input.data.get()), this->_result);
        }

        std::shared_ptr<TNetwork> network;
        MlUtil::SharedArray<InputType> input;
        int inputLength;
    };

    template<typename TNeuralNetwork>
    struct NetworkContext;

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

        auto MakeDecision(MlUtil::SharedArray<InputType> input, int inputLength);

        std::shared_ptr<TNeuralNetwork> network = std::make_shared<TNeuralNetwork>();
        std::shared_ptr<NetworkContext<TNeuralNetwork>> networkContext;
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
            int sampleId = endSampleId - (outSamples.size() - 1 - i);
            bool gotSample = _owner->TryGetSampleById(sampleId, outSamples[i]);
            assert(gotSample);
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

    struct ITrainer
    {
        virtual ~ITrainer() = default;
    };

    template<typename TNeuralNetwork>
    struct Trainer;

    template<typename TNeuralNetwork>
    struct RunTrainingBatchWorkItem : ThreadPoolWorkItem<TrainingBatchResult>
    {
        using InputType = typename TNeuralNetwork::InputType;
        using OutputType = typename TNeuralNetwork::OutputType;
        using NetworkType = TNeuralNetwork;
        using SampleType = Sample<InputType, OutputType>;

        RunTrainingBatchWorkItem(
            std::shared_ptr<TNeuralNetwork> network_,
            std::shared_ptr<Trainer<TNeuralNetwork>> trainer_,
            MlUtil::SharedArray<SampleType> samples_,
            int batchSize_,
            int sequenceLength_)
            : network(network_),
              samples(samples_),
              trainer(trainer_),
              batchSize(batchSize_),
              sequenceLength(sequenceLength_)
        {

        }

        void Execute() override;

        std::shared_ptr<TNeuralNetwork> network;
        MlUtil::SharedArray<SampleType> samples;
        std::shared_ptr<Trainer<TNeuralNetwork>> trainer;
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
        NetworkContext(Decider<TNeuralNetwork>* decider_, Trainer<TNeuralNetwork>* trainer_)
            : decider(decider_),
              trainer(trainer_)
        {

        }

        std::shared_ptr<TNeuralNetwork> SetNewNetwork(std::stringstream& stream)
        {
            newNetworkLock.Lock();
            std::shared_ptr<TNeuralNetwork> result = std::make_shared<TNeuralNetwork>();
            newNetwork = result;
            TorchLoad(newNetwork->module, stream);
            trainer->OnCreateNewNetwork(newNetwork);
            newNetworkLock.Unlock();

            return result;
        }

        std::shared_ptr<TNeuralNetwork> TryGetNewNetwork()
        {
            newNetworkLock.Lock();
            auto result = newNetwork;
            newNetwork = nullptr;
            newNetworkLock.Unlock();
            return result;
        }

        Decider<TNeuralNetwork>* decider;
        Trainer<TNeuralNetwork>* trainer;

        std::shared_ptr<TNeuralNetwork> newNetwork;
        SpinLock newNetworkLock;

        virtual ~NetworkContext() = default;
    };

    template <typename TNeuralNetwork>
    auto Decider<TNeuralNetwork>::MakeDecision(MlUtil::SharedArray<InputType> input, int inputLength)
    {
        auto newNetwork = networkContext->TryGetNewNetwork();
        if (newNetwork != nullptr)
        {
            network = newNetwork;
        }

        auto workItem = std::make_shared<MakeDecisionWorkItem<TNeuralNetwork>>(network, input, inputLength);
        auto threadPool = ThreadPool::GetInstance();
        threadPool->StartItem(workItem);
        return workItem;
    }

    template<typename TNeuralNetwork>
    struct Trainer : ITrainer, std::enable_shared_from_this<Trainer<TNeuralNetwork>>
    {
        using InputType = typename TNeuralNetwork::InputType;
        using OutputType = typename TNeuralNetwork::OutputType;
        using NetworkType = TNeuralNetwork;
        using SampleType = Sample<InputType, OutputType>;

        Trainer(int batchSize_, float trainsPerSecond_)
            : sampleRepository(rng),
              trainingInput(MlUtil::SharedArray<SampleType>(batchSize_ * TNeuralNetwork::SequenceLength)),
              batchSize(batchSize_),
              sequenceLength(TNeuralNetwork::SequenceLength),
              trainsPerSecond(trainsPerSecond_)
        {
            static_assert(std::is_base_of_v<INeuralNetwork, TNeuralNetwork>, "Neural network must inherit from INeuralNetwork<>");
        }

        void AddSample(SampleType& sample)
        {
            sampleLock.Lock();
            ReceiveSample(sample);
            sampleLock.Unlock();

            ++totalSamples;
            if(!isTraining && totalSamples >= minSamplesBeforeStartingTraining)
            {
                isTraining = true;
                RunBatch();
            }
        }

        virtual void RunBatch()
        {
            sampleLock.Lock();
            bool successful = TryCreateBatch(Grid<SampleType>(batchSize, sequenceLength, trainingInput.data.get()));
            sampleLock.Unlock();

            if (successful)
            {
                auto taskScheduler = TaskScheduler::GetInstance();

                float runTime = trainTask != nullptr
                                ? trainTask->startTime + 1.0f / trainsPerSecond
                                : 0;

                // TODO: can save the memory allocation by reusing the task
                trainTask = std::make_shared<ScheduledTask>();
                trainTask->workItem = std::make_shared<RunTrainingBatchWorkItem<TNeuralNetwork>>(network, this->shared_from_this(), trainingInput, batchSize, sequenceLength);
                trainTask->runTime = runTime;
                taskScheduler->Start(trainTask);
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

        void NotifyTrainingComplete(std::stringstream& serializedNetwork, const TrainingBatchResult& result)
        {
            auto newNetwork = networkContext->SetNewNetwork(serializedNetwork);

            OnTrainingComplete(result);

            if(isTraining)
            {
                RunBatch();
            }
        }

        virtual void OnTrainingComplete(const TrainingBatchResult& result) { }
        virtual void ReceiveSample(const SampleType& sample) { }
        virtual bool TrySelectSequenceSamples(gsl::span<SampleType> outSequence) { return false;  }
        virtual void OnCreateNewNetwork(std::shared_ptr<NetworkType> newNetwork) { }

        SpinLock sampleLock;
        RandomNumberGenerator rng;
        SampleRepository<SampleType> sampleRepository;
        MlUtil::SharedArray<SampleType> trainingInput;
        int batchSize;
        int sequenceLength;
        float trainsPerSecond;
        std::shared_ptr<ScheduledTask> trainTask;
        std::shared_ptr<NetworkContext<TNeuralNetwork>> networkContext;
        std::shared_ptr<TNeuralNetwork> network;
        bool isTraining = false;
        int minSamplesBeforeStartingTraining = 32;
        int totalSamples = 0;
    };

    template<typename TNeuralNetwork>
    void RunTrainingBatchWorkItem<TNeuralNetwork>::Execute()
    {
        Grid<const SampleType> input(batchSize, sequenceLength, samples.data.get());
        network->TrainBatch(input, _result);
        std::stringstream stream;
        TorchSave(network->module, stream);
        trainer->NotifyTrainingComplete(stream, _result);
    }

    template<typename T>
    struct Serializer<T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
    {
        static void Serialize(T& value, ObjectSerializer& serializer)
        {
            serializer.template AddBytes(reinterpret_cast<unsigned char*>(&value), sizeof(value));
        }
    };
}