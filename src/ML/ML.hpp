#pragma once

#include "Engine.hpp"
#include "StrifeML.hpp"
#include "Math/Vector2.hpp"
#include "Container/Grid.hpp"
#include "Scene/EntityComponent.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

template<>
struct StrifeML::Serializer<Vector2>
{
    static void Serialize(Vector2& value, StrifeML::ObjectSerializer& serializer)
    {
        Serializer<float>::Serialize(value.x, serializer);
        Serializer<float>::Serialize(value.y, serializer);
    }
};

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

        SensorObject& AllowTriggers()
        {
            allowTriggers = true;
            return *this;
        }

        Color color;
        float priority;
        int id;
        bool allowTriggers = false;
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

        trainer->StartRunning();

        _trainers.emplace(std::move(trainer));
        return trainerPtr;
    }

    template<typename TDecider, typename TTrainer>
    StrifeML::NetworkContext<typename TDecider::NetworkType>* CreateNetwork(const char* name, TDecider* decider, TTrainer* trainer, int sequenceLength)
    {
        static_assert(std::is_same_v<typename TDecider::NetworkType, typename TTrainer::NetworkType>, "Trainer and decider must accept the same type of neural network");

        auto context = _networksByName.find(name);
        if (context != _networksByName.end())
        {
            throw StrifeML::StrifeException("Network already exists: " + std::string(name));
        }

        auto newContext = std::make_shared<StrifeML::NetworkContext<typename TDecider::NetworkType>>(decider, trainer, sequenceLength);
        _networksByName[name] = newContext;

        trainer->networkContext = newContext;
        trainer->network = std::make_shared<typename TDecider::NetworkType>();

        trainer->OnCreateNewNetwork(trainer->network);

        newContext->decider->networkContext = newContext;

        return newContext.get();
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
        StrifeML::Serializer<int>::Serialize(rows, serializer);
        StrifeML::Serializer<int>::Serialize(cols, serializer);

        // Dimensions of serialized grid must match the grid being deserialized into
        assert(rows == Rows);
        assert(cols == Cols);

        StrifeML::Serializer<bool>::Serialize(_isCompressed, serializer);
        if(_isCompressed)
        {
            StrifeML::Serializer<int>::Serialize(_totalRectangles, serializer);
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
    struct Serializer<GridSensorOutput<Rows, Cols>>
    {
        static void Serialize(GridSensorOutput<Rows, Cols>& value, ObjectSerializer& serializer)
        {
            value.Serialize(serializer);
        }
    };
}
