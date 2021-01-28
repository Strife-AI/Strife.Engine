#pragma once
#include <memory>
#include <queue>


#include "Math/Rectangle.hpp"
#include "Math/Vector2.hpp"
#include "Container/Grid.hpp"
#include "Scene/IEntityEvent.hpp"
#include "Scene/Scene.hpp"

struct FlowCell
{
    bool alreadyVisited = false;
    Vector2 dir;
};

struct FlowField
{
    FlowField(int rows, int cols, Vector2 target_)
        : grid(rows, cols),
        target(target_)
    {
        
    }

    VariableSizedGrid<FlowCell> grid;
    Vector2 target;
    PathFinderService* pathFinder;
    Vector2 ClampPosition(const Vector2& position) const;
};

/// <summary>
/// Sent to an entity to inform it that its path request is complete
/// </summary>
DEFINE_EVENT(FlowFieldReadyEvent)
{
    FlowFieldReadyEvent(std::shared_ptr<FlowField> result_)
        : result(result_)
    {
        
    }

    std::shared_ptr<FlowField> result;
};

enum class PathRequestStatus
{
    NotStarted,
    InProgress
};

struct PathRequest
{
    Vector2 start;
    Vector2 end;

    Vector2 startCell;
    Vector2 endCell;

    EntityReference<Entity> owner;
    PathRequestStatus status = PathRequestStatus::NotStarted;
};

enum class ObstacleEdgeFlags : uint8_t
{
    NorthBlocked = 1,
    SouthBlocked = 2,
    EastBlocked = 4,
    WestBlocked = 8
};

struct ObstacleCell
{
    char count = 0;
    Flags<ObstacleEdgeFlags> flags;
};

class PathFinderService : public ISceneService
{
public:
    PathFinderService(int rows, int cols);

    void AddEdge(Vector2 from, Vector2 to);
    void RemoveEdge(Vector2 from, Vector2 to);

    void AddObstacle(const Rectangle& bounds);
    void RemoveObstacle(const Rectangle& bounds);
    void RequestFlowField(Vector2 start, Vector2 end, Entity* owner);
    void Visualize(Renderer* renderer);

    ObstacleCell& GetCell(Vector2 position)
    {
        return _obstacleGrid[position];
    }

private:
    static constexpr int MaxGridCalculationsPerTick = 32768 * 8;

    void ReceiveEvent(const IEntityEvent& ev) override;
    void CalculatePaths();

    Vector2 PixelToCellCoordinate(Vector2 position) const;
    void EnqueueCellIfValid(Vector2 cell, Vector2 from);

    using WorkQueue = std::queue<Vector2>;

    VariableSizedGrid<ObstacleCell> _obstacleGrid;
    std::queue<PathRequest> _requestQueue;

    WorkQueue _workQueue;
    std::shared_ptr<FlowField> _fieldInProgress;
};