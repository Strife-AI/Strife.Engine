#pragma once
#include <memory>
#include <queue>


#include "Math/Rectangle.hpp"
#include "Math/Vector2.hpp"
#include "Container/Grid.hpp"
#include "Scene/IEntityEvent.hpp"
#include "Scene/Scene.hpp"

enum class FlowDirection : unsigned char
{
    North,
    East,
    South,
    West,
    Unset,
    Zero
};

struct FlowCell
{
    FlowDirection direction = FlowDirection::Unset;
};

Vector2 FlowDirectionToVector2(FlowDirection direction);
FlowDirection OppositeFlowDirection(FlowDirection direction);

struct FlowField
{
    FlowField(int rows, int cols, Vector2 target_, Vector2 tileSize_)
        : grid(rows, cols),
        target(target_),
        tileSize(tileSize_)
    {
        
    }

    Vector2 GetFlowDirectionAtCell(Vector2 position);
    Vector2 GetFilteredFlowDirection(Vector2 position);

    VariableSizedGrid<FlowCell> grid;
    Vector2 target;
    Vector2 tileSize;
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

class PathFinderService : public ISceneService
{
public:
    PathFinderService(int rows, int cols, Vector2 tileSize);

    void AddObstacle(const Rectangle& bounds);
    void RequestFlowField(Vector2 start, Vector2 end, Entity* owner);

private:
    static constexpr int MaxGridCalculationsPerTick = 32768;

    void ReceiveEvent(const IEntityEvent& ev) override;
    void CalculatePaths();
    void Visualize(Renderer* renderer);

    Vector2 PixelToCellCoordinate(Vector2 position) const;
    void EnqueueCellIfValid(Vector2 cell, FlowDirection fromDirection);

    using WorkQueue = std::queue<Vector2>;

    VariableSizedGrid<unsigned char> _obstacleGrid;
    Vector2 _tileSize;
    std::queue<PathRequest> _requestQueue;

    WorkQueue _workQueue;
    std::shared_ptr<FlowField> _fieldInProgress;
};