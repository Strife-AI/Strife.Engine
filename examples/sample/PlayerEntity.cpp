#include "PlayerEntity.hpp"
#include "InputService.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Net/ReplicationManager.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"
#include "Scene/TilemapEntity.hpp"

#include "torch/torch.h"

template<int TotalDimensions>
struct Dimensions
{
    template<typename ...Args>
    constexpr Dimensions(Args&& ...dims)
        : dimensions{dims...}
    {

    }

    template<int RhsTotalDimensions>
    constexpr Dimensions<TotalDimensions + RhsTotalDimensions> Union(const Dimensions<RhsTotalDimensions>& rhs) const
    {
        Dimensions<TotalDimensions + RhsTotalDimensions> result;
        for(int i = 0; i < TotalDimensions; ++i) result.dimensions[i] = dimensions[i];
        for(int i = 0; i < RhsTotalDimensions; ++i) result.dimensions[i + TotalDimensions] = rhs.dimensions[i];
        return result;
    }

    static int GetTotalDimensions()
    {
        return TotalDimensions;
    }

    int64_t dimensions[TotalDimensions] { 0 };
};

template<typename T>
struct DimensionCalculator
{

};

template<>
struct DimensionCalculator<int>
{
    static constexpr auto Dims(const int& value)
    {
        return Dimensions<1>(1);
    }
};

template<typename TCell>
struct DimensionCalculator<Grid<TCell>>
{
    static constexpr auto Dims(const Grid<TCell>& grid)
    {
        return Dimensions<2>(grid.Rows(), grid.Cols()).Union(DimensionCalculator<TCell>::Dims(grid[0][0]));
    }
};

template<int Rows, int Cols>
struct DimensionCalculator<GridSensorOutput<Rows, Cols>>
{
    static constexpr auto Dims(const GridSensorOutput<Rows, Cols>& grid)
    {
        return Dimensions<2>(Rows, Cols).Union(DimensionCalculator<int>::Dims(0));
    }
};

template<typename ...TArgs>
struct DimensionCalculator<std::tuple<TArgs...>>
{

};

template<typename T>
struct GetCellType
{
    using Type = T;
};

template<typename TCell>
struct GetCellType<Grid<TCell>>
{
    using Type = typename GetCellType<TCell>::Type;
};

template<int Rows, int Cols>
struct GetCellType<GridSensorOutput<Rows, Cols>>
{
    using Type = int;
};

template<typename T>
c10::ScalarType GetTorchType();

template<> c10::ScalarType GetTorchType<int>() { return torch::kInt32; }
template<> c10::ScalarType GetTorchType<float>() { return torch::kFloat32; }
template<> c10::ScalarType GetTorchType<uint64_t>() { return torch::kInt64; }
template<> c10::ScalarType GetTorchType<double>() { return torch::kFloat64; }

template<typename T, typename TorchType = T>
struct TorchPacker
{

};

template<>
struct TorchPacker<int>
{
    static int* Pack(const int& value, int* outPtr)
    {
        *outPtr = value;
        return outPtr + 1;
    }
};

template<typename TCell, typename TorchType>
struct TorchPacker<Grid<TCell>, TorchType>
{
    static TorchType* Pack(const Grid<TCell>& value, TorchType* outPtr)
    {
        if constexpr (std::is_arithmetic_v<TCell>)
        {
            memcpy(outPtr, &value[0][0], value.Rows() * value.Cols() * sizeof(TCell));
            return outPtr + value.Rows() * value.Cols();
        }
        else
        {
            for (int i = 0; i < value.Rows(); ++i)
            {
                for (int j = 0; j < value.Cols(); ++j)
                {
                    outPtr = TorchPacker<TCell, TorchType>::Pack(value[i][j], outPtr);
                }
            }

            return outPtr;
        }
    }
};

template<int Rows, int Cols>
struct TorchPacker<GridSensorOutput<Rows, Cols>, int>
{
    static int* Pack(const GridSensorOutput<Rows, Cols>& value, int* outPtr)
    {
        if(value.IsCompressed())
        {
            FixedSizeGrid<int, Rows, Cols> decompressedGrid;
            value.Decompress(decompressedGrid);
            return TorchPacker<Grid<int>, int>::Pack(decompressedGrid, outPtr);
        }
        else
        {
            Grid<int> grid(Rows, Cols, const_cast<int*>(value.GetRawData()));
            return TorchPacker<Grid<int>, int>::Pack(grid, outPtr);
        }
    }
};

template<typename T>
torch::Tensor PackIntoTensor(const T& value)
{
    using CellType = typename GetCellType<T>::Type;
    auto dimensions = DimensionCalculator<T>::Dims(value);

    torch::IntArrayRef dims(dimensions.dimensions, dimensions.GetTotalDimensions());
    auto torchType = GetTorchType<CellType>();
    auto t = torch::empty(dims, torchType);

    TorchPacker<T, CellType>::Pack(value, t.template data_ptr<CellType>());

    return t.squeeze();
}

template<int Rows, int Cols>
torch::Tensor PackIntoTensor(GridSensorOutput<Rows, Cols>& value)
{
    FixedSizeGrid<int, Rows, Cols> grid;
    value.Decompress(grid);
    return PackIntoTensor((const Grid<int>&)grid);
}

template<typename T, typename TSelector>
torch::Tensor PackIntoTensor(const Grid<T>& grid, TSelector selector)
{
    using SelectorReturnType = decltype(selector(grid[0][0]));
    using CellType = typename GetCellType<SelectorReturnType>::Type;

    SelectorReturnType selectorTemp;
    Grid<SelectorReturnType> dummyGrid(grid.Rows(), grid.Cols(), &selectorTemp);

    auto dimensions = DimensionCalculator<Grid<SelectorReturnType>>::Dims(dummyGrid);

    torch::IntArrayRef dims(dimensions.dimensions, dimensions.GetTotalDimensions());
    auto torchType = GetTorchType<CellType>();
    auto t = torch::empty(dims, torchType);

    auto outPtr = t.template data_ptr<CellType>();

    for(int i = 0; i < grid.Rows(); ++i)
    {
        for(int j = 0; j < grid.Cols(); ++j)
        {
            auto selectedValue = selector(grid[i][j]);
            outPtr = TorchPacker<SelectorReturnType, CellType>::Pack(selectedValue, outPtr);
        }
    }

    return t.squeeze();
}

struct Value
{
    GridSensorOutput<40, 40> grid;
    Vector2 velocity;
    float groundDistance;
    int i;
};

void Test()
{
    FixedSizeGrid<Value, 32, 5> grid;

    // 32 x 5 x 40 x 40 float
    auto spacialInput = PackIntoTensor(
        grid,
        [=](auto& val)  { return val.grid; });

    // 32 x 5 x 3 float
    auto featureInput = PackIntoTensor(
        grid,
        [=](auto& val) { return std::make_tuple(val.velocity.x, val.velocity.y, val.groundDistance); });

    // 32 x 5 int
    auto otherInput = PackIntoTensor(
        grid,
        [=](auto& val) { return val.i; });
}

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    Test();

    rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    auto box = rigidBody->CreateBoxCollider(Dimensions());

    box->SetDensity(1);
    box->SetFriction(0);

    net = AddComponent<NetComponent>();

    scene->SendEvent(PlayerAddedToGame(this));

    scene->GetService<InputService>()->players.push_back(this);

    // Setup network and sensors
    {
        auto nn = AddComponent<NeuralNetworkComponent<PlayerNetwork>>();
        nn->SetNetwork("nn");

        // Network only runs on server
        if (scene->isServer) nn->mode = NeuralNetworkMode::Deciding;

        auto gridSensor = AddComponent<GridSensorComponent<40, 40>>("grid", Vector2(16, 16));
        gridSensor->render = true;

        // Called when:
        //  * Collecting input to make a decision
        //  * Adding a training sample
        nn->collectInput = [=](PlayerModelInput& input)
        {
            input.velocity = rigidBody->GetVelocity();
            gridSensor->Read(input.grid);
        };

        // Called when the decider makes a decision
        nn->receiveDecision = [=](PlayerDecision& decision)
        {

        };

        // Collects what decision the player made
        nn->collectDecision = [=](PlayerDecision& outDecision)
        {
            outDecision.action = PlayerAction::Down;
        };
    }
}

void PlayerEntity::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<SpawnedOnClientEvent>())
    {
        if (net->ownerClientId == scene->replicationManager->localClientId)
        {
            scene->GetCameraFollower()->FollowMouse();
            scene->GetCameraFollower()->CenterOn(Center());
            scene->GetService<InputService>()->activePlayer = this;
        }
    }
}

void PlayerEntity::ReceiveServerEvent(const IEntityEvent& ev)
{
    if (auto flowFieldReady = ev.Is<FlowFieldReadyEvent>())
    {
        net->flowField = flowFieldReady->result;
    }
    else if (auto moveTo = ev.Is<MoveToEvent>())
    {
        scene->GetService<PathFinderService>()->RequestFlowField(Center(), moveTo->position, this);
        state = PlayerState::Moving;
    }
    else if (auto attack = ev.Is<AttackEvent>())
    {
        attackTarget = attack->entity;
        updateTargetTimer = 0;
        state = PlayerState::Attacking;
    }
}

void PlayerEntity::OnDestroyed()
{
    scene->SendEvent(PlayerRemovedFromGame(this));
}

void PlayerEntity::Render(Renderer* renderer)
{
    auto position = Center();

    Color c[3] = {
        Color::CornflowerBlue(),
        Color::Green(),
        Color::Orange()
    };

    auto color = c[net->ownerClientId];

    renderer->RenderRectangle(Rectangle(position - Dimensions() / 2, Dimensions()), color, -0.99);

    Vector2 healthBarSize(32, 4);
    renderer->RenderRectangle(Rectangle(
        Center() - Vector2(0, 20) - healthBarSize / 2,
        Vector2(healthBarSize.x * health.currentValue / 100, healthBarSize.y)),
        Color::White(),
        -1);

    if (showAttack.currentValue)
    {
        renderer->RenderLine(Center(), attackPosition.currentValue, Color::Red(), -1);
    }
}

void PlayerEntity::ServerFixedUpdate(float deltaTime)
{
    auto client = net;

    attackCoolDown -= deltaTime;

    if(state == PlayerState::Attacking)
    {
        Entity* target;
        RaycastResult hitResult;
        if(attackTarget.TryGetValue(target)
            && (target->Center() - Center()).Length() < 200
            && scene->Raycast(Center(), target->Center(), hitResult)
            && hitResult.handle.OwningEntity() == target)
        {
            rigidBody->SetVelocity({ 0, 0 });

            if (attackCoolDown <= 0)
            {
                PlayerEntity* player;
                if (target->Is<PlayerEntity>(player))
                {
                    player->health.currentValue -= 100;

                    if (player->health.currentValue <= 0)
                    {
                        player->Destroy();
                    }

                    attackCoolDown = 3;
                    showAttack = true;
                    StartTimer(0.3, [=]
                    {
                        showAttack = false;
                    });
                }
            }

            return;
        }
    }

    if (client->flowField != nullptr)
    {
        Vector2 velocity;

        Vector2 points[4];
        client->owner->Bounds().GetPoints(points);

        bool useBeeLine = true;
        for (auto p : points)
        {
            RaycastResult result;
            if (scene->Raycast(p, client->flowField->target, result)
                && (state != PlayerState::Attacking || result.handle.OwningEntity() != attackTarget.GetValueOrNull()))
            {
                useBeeLine = false;
                break;
            }
        }

        if (useBeeLine)
        {
            velocity = (client->flowField->target - client->owner->Center()).Normalize() * 200;
        }
        else
        {
            velocity = client->flowField->GetFilteredFlowDirection(client->owner->Center() - Vector2(16, 16)) * 200;

        }

        velocity = client->owner->GetComponent<RigidBodyComponent>()->GetVelocity().SmoothDamp(
            velocity,
            client->acceleration,
            0.05,
            Scene::PhysicsDeltaTime);

        if ((client->owner->Center() - client->flowField->target).Length() < 200 * Scene::PhysicsDeltaTime)
        {
            velocity = { 0, 0 };
            client->flowField = nullptr;
        }

        client->owner->GetComponent<RigidBodyComponent>()->SetVelocity(velocity);
        //Renderer::DrawDebugLine({ client->owner->Center(), client->owner->Center() + velocity, Color::Red() });
    }
}

void PlayerEntity::ServerUpdate(float deltaTime)
{
    if (state == PlayerState::Attacking)
    {
        Entity* target;
        if (attackTarget.TryGetValue(target))
        {
            attackPosition = target->Center();
            updateTargetTimer -= deltaTime;
            if (updateTargetTimer <= 0)
            {
                scene->GetService<PathFinderService>()->RequestFlowField(Center(), target->Center(), this);
                updateTargetTimer = 2;
            }
            else if (net->flowField != nullptr)
            {
                net->flowField->target = target->Center();
            }
        }
        else
        {
            state = PlayerState::None;
            net->flowField = nullptr;
        }
    }
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}

