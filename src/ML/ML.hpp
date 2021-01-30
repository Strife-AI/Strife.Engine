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
    Disabled,
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

    explicit NeuralNetworkComponent(float decisionsPerSecond_ = 1)
        : decisionInput(StrifeML::MlUtil::SharedArray<InputType>(TNeuralNetwork::SequenceLength)),
          decisionsPerSecond(decisionsPerSecond_)
    {

    }

    virtual ~NeuralNetworkComponent() = default;

    void Update(float deltaTime) override;

    void SetNetwork(const char* name);

    void MakeDecision();

    StrifeML::NetworkContext<NetworkType>* networkContext = nullptr;
    std::shared_ptr<StrifeML::MakeDecisionWorkItem<TNeuralNetwork>> decisionInProgress;
    StrifeML::MlUtil::SharedArray<InputType> decisionInput;
    float makeDecisionsTimer = 0;
    float decisionsPerSecond;

    std::shared_ptr<StrifeML::RunTrainingBatchWorkItem<TNeuralNetwork>> trainingInProgress;

    std::function<void(InputType& input)> collectInput;
    std::function<void(OutputType& decision)> collectDecision;

    std::function<void(OutputType& decision)> receiveDecision;

    NeuralNetworkMode mode = NeuralNetworkMode::Disabled;
};

template <typename TNeuralNetwork>
void NeuralNetworkComponent<TNeuralNetwork>::Update(float deltaTime)
{
    if (mode == NeuralNetworkMode::Disabled)
    {
        return;
    }
    else if (mode == NeuralNetworkMode::Deciding)
    {
        OutputType output;
        if (decisionInProgress != nullptr && decisionInProgress->TryGetResult(output))
        {
            if (receiveDecision != nullptr)
            {
                receiveDecision(output);
            }

            decisionInProgress = nullptr;
        }

        makeDecisionsTimer -= deltaTime;

        if (makeDecisionsTimer <= 0)
        {
            makeDecisionsTimer = 0;
            MakeDecision();
        }
    }
    else if (mode == NeuralNetworkMode::CollectingSamples)
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
        decisionInput.data.get()[i] = std::move(decisionInput.data.get()[i + 1]);
    }

    // Collect new input
    collectInput(decisionInput.data.get()[TNeuralNetwork::SequenceLength - 1]);

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

    template<typename TTrainer, typename ... Args>
    TTrainer* CreateTrainer(Args&& ... constructorArgs)
    {
        static_assert(std::is_base_of_v<StrifeML::ITrainer, TTrainer>, "Trainer must inherit from ITrainer<TInput, TOutput>");
        auto trainer = std::make_shared<TTrainer>(std::forward<Args>(constructorArgs) ...);
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

        auto newContext = std::make_shared<StrifeML::NetworkContext<typename TDecider::NetworkType>>(decider, trainer);
        _networksByName[name] = newContext;
        newContext->trainer->networkContext = newContext;
        newContext->decider->networkContext = newContext;
    }

    // TODO remove network method

    template<typename TNeuralNetwork>
    StrifeML::NetworkContext<TNeuralNetwork>* GetNetwork(const char* name)
    {
        // TODO error handling if missing or wrong type
        return dynamic_cast<StrifeML::NetworkContext<TNeuralNetwork>*>(_networksByName[name].get());
    }

    static void SetSensorObjectDefinition(const SensorObjectDefinition& definition)
    {
        _sensorObjectDefinition = std::make_shared<SensorObjectDefinition>(definition);
    }

    static std::shared_ptr<SensorObjectDefinition> GetSensorObjectDefinition()
    {
        return _sensorObjectDefinition;
    }

private:
    static std::shared_ptr<SensorObjectDefinition> _sensorObjectDefinition;

    std::unordered_set<std::unique_ptr<StrifeML::IDecider>> _deciders;
    std::unordered_set<std::shared_ptr<StrifeML::ITrainer>> _trainers;
    std::unordered_map<std::string, std::shared_ptr<StrifeML::INetworkContext>> _networksByName;
};

template<typename TNeuralNetwork>
void NeuralNetworkComponent<TNeuralNetwork>::SetNetwork(const char* name)
{
    auto nnManager = this->owner->GetEngine()->GetNeuralNetworkManager();
    networkContext = nnManager->template GetNetwork<TNeuralNetwork>(name);
}

gsl::span<uint64_t> ReadGridSensorRectangles(
    Scene* scene,
    Vector2 center,
    Vector2 cellSize,
    int rows,
    int cols,
    SensorObjectDefinition* objectDefinition,
    Entity* self);

void DecompressGridSensorOutput(gsl::span<const uint64_t> compressedRectangles, Grid<uint64_t>& outGrid, SensorObjectDefinition* objectDefinition);

template<int Rows, int Cols>
struct GridSensorOutput
{
    GridSensorOutput()
        : _isCompressed(true),
          _totalRectangles(0)
    {

    }

    GridSensorOutput(const GridSensorOutput& rhs)
    {
        Assign(rhs);
    }

    GridSensorOutput& operator=(const GridSensorOutput& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        Assign(rhs);
        return *this;
    }

    void SetRectangles(gsl::span<uint64_t> rectangles)
    {
        if (rectangles.size() <= MaxCompressedRectangles)
        {
            _isCompressed = true;
            _totalRectangles = rectangles.size();
            memcpy(compressedRectangles, rectangles.data(), rectangles.size_bytes());
        }
        else
        {
            _isCompressed = false;
            Grid<uint64_t> grid(Rows, Cols, &data[0][0]);
            DecompressGridSensorOutput(rectangles, grid, NeuralNetworkManager::GetSensorObjectDefinition().get());
        }
    }

    void Decompress(Grid<uint64_t>& outGrid) const
    {
        if(_isCompressed)
        {
            DecompressGridSensorOutput(gsl::span<const uint64_t>(compressedRectangles, _totalRectangles), outGrid, NeuralNetworkManager::GetSensorObjectDefinition().get());
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

    const uint64_t* GetRawData() const
    {
        return &data[0][0];
    }

    bool IsCompressed() const
    {
        return _isCompressed;
    }

    void Serialize(StrifeML::ObjectSerializer& serializer)
    {
        int rows = Rows;
        int cols = Cols;
        serializer.Add(rows);
        serializer.Add(cols);

        // Dimensions of serialized grid must match the grid being deserialized into
        assert(rows == Rows);
        assert(cols == Cols);

        serializer.Add(_isCompressed);
        if(_isCompressed)
        {
            serializer.Add(_totalRectangles);
            serializer.AddBytes(compressedRectangles, _totalRectangles);
        }
        else
        {
            serializer.AddBytes(&data[0][0], Rows * Cols);
        }
    }

private:
    void Assign(const GridSensorOutput& rhs)
    {
        _isCompressed = rhs._isCompressed;
        _totalRectangles = rhs._totalRectangles;

        if(_isCompressed)
        {
            memcpy(compressedRectangles, rhs.compressedRectangles, sizeof(uint64_t) * rhs._totalRectangles);
        }
        else
        {
            memcpy(data, rhs.data, sizeof(uint64_t) * Rows * Cols);
        }
    }

    bool _isCompressed;
    int _totalRectangles;

    static constexpr int MaxCompressedRectangles = NearestPowerOf2(sizeof(uint64_t) * Rows * Cols, sizeof(uint64_t)) / sizeof(uint64_t);

    union
    {
        uint64_t data[Rows][Cols];
        uint64_t compressedRectangles[MaxCompressedRectangles];
    };
};

void RenderGridSensorOutput(Grid<uint64_t>& grid, Vector2 center, Vector2 cellSize, SensorObjectDefinition* objectDefinition, Renderer* renderer, float depth);
void RenderGridSensorOutputIsometric(Grid<uint64_t>& grid, Vector2 center, Vector2 cellSize, SensorObjectDefinition* objectDefinition, Renderer* renderer, float depth);

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
        return this->owner->Center() + offsetFromEntityCenter;
    }

    void OnAdded() override
    {
        sensorObjectDefinition = this->GetScene()->GetEngine()->GetNeuralNetworkManager()->GetSensorObjectDefinition();
    }

    void Read(SensorOutput& output)
    {
        auto sensorGridRectangles = ReadGridSensorRectangles(
            this->GetScene(),
            GridCenter(),
            cellSize,
            Rows,
            Cols,
            sensorObjectDefinition.get(),
            this->owner);

        output.SetRectangles(sensorGridRectangles);
    }

    void Render(Renderer* renderer) override
    {
        if (render)
        {
            FixedSizeGrid<uint64_t, Rows, Cols> decompressed;
            SensorOutput output;
            Read(output);
            output.Decompress(decompressed);

            if (this->GetScene()->perspective == ScenePerspective::Orothgraphic)
            {
                RenderGridSensorOutput(decompressed, GridCenter(), cellSize, sensorObjectDefinition.get(), renderer, -1);
            }
            else if (this->GetScene()->perspective == ScenePerspective::Isometric)
            {
                RenderGridSensorOutputIsometric(decompressed, GridCenter(), cellSize, sensorObjectDefinition.get(), renderer, -1);
            }
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
        value.Serialize(serializer);
    }
}
