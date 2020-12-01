#pragma once
#include "Engine.hpp"
#include "../../Strife.ML/NewStuff.hpp"
#include "Math/Vector2.hpp"
#include "Scene/EntityComponent.hpp"
#include "Scene/Entity.hpp"

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

struct SensorObjectDefinition
{
    struct SensorObject
    {
        SensorObject& SetColor(Color color_)
        {
            color = color_;
            return *this;
        }

        SensorObject& SetPriority(float priority_)
        {
            priority = priority_;
            return *this;
        }

        Color color;
        float priority;
        unsigned int id;
    };

    template<typename TEntity>
    SensorObject& Add(int id)
    {
        static_assert(std::is_base_of_v<Entity, TEntity>, "TEntity must be an entity defined with DEFINE_ENTITY()");
        SensorObject& object = objectByType[TEntity::Type.key];
        object.priority = nextPriority;
        object.color = Color::White();
        object.id = id;
        return object;
    }

    std::unordered_map<unsigned int, SensorObject> objectByType;
    float nextPriority = 1;
};

struct NeuralNetworkManager
{
    template<typename TDecider>
    TDecider* CreateDecider()
    {
        static_assert(std::is_base_of_v<StrifeML::IDecider, TDecider>, "Decider must inherit from IDecider<TInput, TOutput>");
        auto decider = std::make_unique<TDecider>();
        auto deciderPtr = decider.get();
        _deciders.emplace(std::move(decider));
        return deciderPtr;
    }

    template<typename TTrainer>
    TTrainer* CreateTrainer()
    {
        static_assert(std::is_base_of_v<StrifeML::ITrainerInternal, TTrainer>, "Trainer must inherit from ITrainer<TInput, TOutput>");
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
            throw StrifeML::StrifeException("Network already exists: " + std::string(name));
        }

        _networksByName[name] = std::make_shared<StrifeML::NetworkContext<typename TDecider::NetworkType>>(decider, trainer);
    }

    // TODO remove network method

    template<typename TNeuralNetwork>
    StrifeML::NetworkContext<TNeuralNetwork>* GetNetwork(const char* name)
    {
        // TODO error handling if missing or wrong type
        return dynamic_cast<StrifeML::NetworkContext<TNeuralNetwork>*>(_networksByName[name].get());
    }

    void SetSensorObjectDefinition(const SensorObjectDefinition& definition)
    {
        _sensorObjectDefinition = std::make_shared<SensorObjectDefinition>(definition);
    }

    std::shared_ptr<SensorObjectDefinition> GetSensorObjectDefinition() const
    {
        return _sensorObjectDefinition;
    }

private:
    std::unordered_set<std::unique_ptr<StrifeML::IDecider>> _deciders;
    std::unordered_set<std::unique_ptr<StrifeML::ITrainerInternal>> _trainers;
    std::unordered_map<std::string, std::shared_ptr<StrifeML::INetworkContext>> _networksByName;
    std::shared_ptr<SensorObjectDefinition> _sensorObjectDefinition = std::make_shared<SensorObjectDefinition>();
};

gsl::span<uint64_t> ReadGridSensorRectangles(
    Scene* scene,
    Vector2 center,
    Vector2 cellSize,
    int rows,
    int cols,
    std::shared_ptr<SensorObjectDefinition>& objectDefinition);

template<int Rows, int Cols>
struct GridSensorOutput
{
    GridSensorOutput()
        : _isCompressed(true),
        _totalRectangles(0)
    {

    }

    void SetRectangles(gsl::span<uint64_t> rectangles)
    {
        if (rectangles.size() <= MaxCompressedRectangles)
        {
            _isCompressed = true;
            _totalRectangles = rectangles.size();
            memcpy(compressedRectangles, rectangles.data(), rectangles.size_bytes());
        }
    }

private:
    bool _isCompressed;
    int _totalRectangles;

    static constexpr int MaxCompressedRectangles = NearestPowerOf2(sizeof(int) * Rows * Cols, sizeof(uint64_t)) / sizeof(uint64_t);

    union
    {
        int data[Rows][Cols];
        uint64_t compressedRectangles[MaxCompressedRectangles];
    };
};

template<int Rows, int Cols>
struct GridSensorComponent : IEntityComponent
{
    using SensorOutput = GridSensorOutput<Rows, Cols>;

    void Read(SensorOutput& output)
    {
        auto sensorGridRectangles = ReadGridSensorRectangles(
            GetScene(),
            owner->Center() + offsetFromEntityCenter,
            cellSize,
            Rows,
            Cols,
            sensorObjectDefinition);

        output.SetRectangles(sensorGridRectangles);
    }

    Vector2 offsetFromEntityCenter;
    Vector2 cellSize;
    std::shared_ptr<SensorObjectDefinition> sensorObjectDefinition;
};