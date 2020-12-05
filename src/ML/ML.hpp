#pragma once
#include "Engine.hpp"
#include "../../Strife.ML/NewStuff.hpp"
#include "Math/Vector2.hpp"
#include "Container/Grid.hpp"
#include "Scene/EntityComponent.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

template<>
inline void StrifeML::Serialize<Vector2>(Vector2& value, StrifeML::ObjectSerializer& serializer)
{
    serializer.Add(value.x).Add(value.y);
}

enum class NeuralNetworkMode
{
    Deciding,
    CollectingSamples
};

template<typename TNeuralNetwork>
struct NeuralNetworkComponent : ComponentTemplate<NeuralNetworkComponent<TNeuralNetwork>>
{
    using InputType = typename TNeuralNetwork::InputType;
    using OutputType = typename TNeuralNetwork::OutputType;
    using NetworkType = TNeuralNetwork;
    using SampleType = StrifeML::Sample<InputType, OutputType>;

    NeuralNetworkComponent(float decisionsPerSecond_ = 1)
        : decisionInput(StrifeML::MlUtil::MakeSharedArray<InputType>(TNeuralNetwork::SequenceLength)),
        decisionsPerSecond(decisionsPerSecond_),
        batchSize(TNeuralNetwork::SequenceLength)
    {

    }

    virtual ~NeuralNetworkComponent() = default;

    void OnAdded() override
    {
        // Disabled by default on the client
        isEnabled = GetScene()->isServer;
    }

    void Update(float deltaTime) override;

    void SetNetwork(const char* name)
    {
        auto nnManager = Engine::GetInstance()->GetNeuralNetworkManager();
        networkContext = nnManager->GetNetwork<TNeuralNetwork>(name);
    }

    void MakeDecision();

    StrifeML::NetworkContext<NetworkType>* networkContext = nullptr;
    std::shared_ptr<StrifeML::MakeDecisionWorkItem<TNeuralNetwork>> decisionInProgress;
    std::shared_ptr<InputType[]> decisionInput;
    float makeDecisionsTimer = 0;
    float decisionsPerSecond;

    std::shared_ptr<StrifeML::RunTrainingBatchWorkItem<TNeuralNetwork>> trainingInProgress;

    std::function<void(InputType& input)> collectInput;
    std::function<void(OutputType& decision)> collectDecision;

    std::function<void(OutputType& decision)> receiveDecision;

    int batchSize;

    bool isCollectingSamples = false;
    bool isMakingDecisions = false;
    
    bool isEnabled = false;
};

template <typename TNeuralNetwork>
void NeuralNetworkComponent<TNeuralNetwork>::Update(float deltaTime)
{
    if(!isEnabled)
    {
        return;
    }

    if (isMakingDecisions)
    {
        makeDecisionsTimer -= deltaTime;

        if (makeDecisionsTimer <= 0)
        {
            makeDecisionsTimer = 0;
            MakeDecision();
        }

        OutputType output;
        if (decisionInProgress != nullptr && decisionInProgress->TryGetResult(output))
        {
            if (receiveDecision != nullptr)
            {
                receiveDecision(output);
            }

            decisionInProgress = nullptr;
        }
    }

    if (isCollectingSamples)
    {
        SampleType sample;
        collectInput(sample.input);
        collectDecision(sample.output);
        networkContext->trainer->AddSample(sample);
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

    if (collectInput == nullptr
        || networkContext == nullptr
        || networkContext->decider == nullptr)
    {
        return;
    }

    // Expire oldest input
    for (int i = 0; i < TNeuralNetwork::SequenceLength - 1; ++i)
    {
        decisionInput[i] = std::move(decisionInput[i + 1]);
    }

    // Collect new input
    collectInput(decisionInput[TNeuralNetwork::SequenceLength - 1]);

    // Start making decision
    decisionInProgress = networkContext->decider->MakeDecision(decisionInput, TNeuralNetwork::SequenceLength);
}

struct SensorObjectDefinition
{
    SensorObjectDefinition()
    {
        entityIdToObjectId[0] = 0;
        objectById[0] = SensorObject();
        objectById[0].id = 0;
        objectById[0].priority = -1000000;
    }

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
        int id;
    };

    template<typename TEntity>
    SensorObject& Add(int id)
    {
        // TODO check for duplicate keys
        static_assert(std::is_base_of_v<Entity, TEntity>, "TEntity must be an entity defined with DEFINE_ENTITY()");
        SensorObject& object = objectById[id];
        object.priority = nextPriority;
        object.color = Color::White();
        object.id = id;
        maxId = Max(maxId, id);

        entityIdToObjectId[TEntity::Type.key] = id;

        return object;
    }

    
    SensorObject& GetEntitySensorObject(StringId key)
    {
        auto it = entityIdToObjectId.find(key.key);
        if(it == entityIdToObjectId.end())
        {
            return objectById[0];
        }
        else
        {
            return objectById[it->second];
        }
    }

    int maxId = 0;

    std::unordered_map<unsigned int, int> entityIdToObjectId;
    std::unordered_map<int, SensorObject> objectById;

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
        auto trainer = std::make_shared<TTrainer>();
        auto trainerPtr = trainer.get();
        _trainers.emplace(std::move(trainer));
        return trainerPtr;
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
    std::unordered_set<std::shared_ptr<StrifeML::ITrainerInternal>> _trainers;
    std::unordered_map<std::string, std::shared_ptr<StrifeML::INetworkContext>> _networksByName;
    std::shared_ptr<SensorObjectDefinition> _sensorObjectDefinition = std::make_shared<SensorObjectDefinition>();
};

gsl::span<uint64_t> ReadGridSensorRectangles(
    Scene* scene,
    Vector2 center,
    Vector2 cellSize,
    int rows,
    int cols,
    SensorObjectDefinition* objectDefinition,
    Entity* self);

void DecompressGridSensorOutput(gsl::span<uint64_t> compressedRectangles, Grid<int>& outGrid, SensorObjectDefinition* objectDefinition);

template<int Rows, int Cols>
struct GridSensorOutput
{
    GridSensorOutput()
        : _isCompressed(true),
        _totalRectangles(0)
    {

    }

    void SetRectangles(gsl::span<uint64_t> rectangles, std::shared_ptr<SensorObjectDefinition> sensorObjectDefinition)
    {
        _sensorObjectDefinition = sensorObjectDefinition;

        if (rectangles.size() <= MaxCompressedRectangles)
        {
            _isCompressed = true;
            _totalRectangles = rectangles.size();
            memcpy(compressedRectangles, rectangles.data(), rectangles.size_bytes());
        }
        else
        {
            _isCompressed = false;
            Grid<int> grid(Rows, Cols, &data[0][0]);
            DecompressGridSensorOutput(rectangles, grid, sensorObjectDefinition.get());
        }
    }

    void Decompress(Grid<int>& outGrid)
    {
        if(_isCompressed)
        {
            DecompressGridSensorOutput(gsl::span<uint64_t>(compressedRectangles, _totalRectangles), outGrid, _sensorObjectDefinition.get());
        }
        else
        {
            if(outGrid.Rows() == Rows && outGrid.Cols() == Cols)
            {
                memcpy(outGrid.Data(), data, sizeof(data));
            }
            else
            {
                outGrid.FillWithZero();
                for(int i = 0; i < Min(Rows, outGrid.Rows()); ++i)
                {
                    for(int j = 0; j < Min(Cols, outGrid.Cols()); ++j)
                    {
                        outGrid[i][j] = data[i][j];
                    }
                }
            }
        }
    }

private:
    bool _isCompressed;
    int _totalRectangles;
    std::shared_ptr<SensorObjectDefinition> _sensorObjectDefinition;

    static constexpr int MaxCompressedRectangles = NearestPowerOf2(sizeof(int) * Rows * Cols, sizeof(uint64_t)) / sizeof(uint64_t);

    union
    {
        int data[Rows][Cols];
        uint64_t compressedRectangles[MaxCompressedRectangles];
    };
};

void RenderGridSensorOutput(Grid<int>& grid, Vector2 center, Vector2 cellSize, SensorObjectDefinition* objectDefinition, Renderer* renderer, float depth);

template<int Rows, int Cols>
struct GridSensorComponent : ComponentTemplate<GridSensorComponent<Rows, Cols>>
{
    using SensorOutput = GridSensorOutput<Rows, Cols>;

    GridSensorComponent(Vector2 cellSize = Vector2(32, 32))
    {
        this->cellSize = cellSize;
    }

    Vector2 GridCenter() const
    {
        return owner->Center() + offsetFromEntityCenter;
    }

    void OnAdded() override
    {
        sensorObjectDefinition = GetScene()->GetEngine()->GetNeuralNetworkManager()->GetSensorObjectDefinition();
    }

    void Read(SensorOutput& output)
    {
        auto sensorGridRectangles = ReadGridSensorRectangles(
            GetScene(),
            GridCenter(),
            cellSize,
            Rows,
            Cols,
            sensorObjectDefinition.get(),
            owner);

        output.SetRectangles(sensorGridRectangles, sensorObjectDefinition);
    }

    void Render(Renderer* renderer) override
    {
        if (render)
        {
            FixedSizeGrid<int, Rows, Cols> decompressed;
            SensorOutput output;
            Read(output);
            output.Decompress(decompressed);
            RenderGridSensorOutput(decompressed, GridCenter(), cellSize, sensorObjectDefinition.get(), renderer, -1);
        }
    }
         
    Vector2 offsetFromEntityCenter;
    Vector2 cellSize;
    std::shared_ptr<SensorObjectDefinition> sensorObjectDefinition;
    bool render = false;
};

namespace StrifeML
{
    template<int Rows, int Cols>
    void Serialize(GridSensorOutput<Rows, Cols>& value, ObjectSerializer& serializer)
    {
        // TODO
    }
}
