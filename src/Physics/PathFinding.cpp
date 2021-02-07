#include "PathFinding.hpp"
#include "Renderer.hpp"
#include "Components/PathFollowerComponent.hpp"

Vector2 FlowField::ClampPosition(const Vector2& position) const
{
    return position
        .Floor()
        .AsVectorOfType<float>()
        .Clamp({ 0.0f, 0.0f }, grid.Dimensions() - Vector2(1));
}

PathFinderService::PathFinderService(int rows, int cols)
    : _obstacleGrid(rows, cols)
{
    _obstacleGrid.FillWithZero();
}

void PathFinderService::AddObstacle(const Rectangle& bounds)
{
    auto topLeft = PixelToCellCoordinate(bounds.TopLeft()).Max({ 0, 0 });
    auto bottomRight = PixelToCellCoordinate(bounds.BottomRight());

    if(!IsApproximately(bounds.BottomRight().x, floor(bounds.BottomRight().x)))
    {
        ++bottomRight.x;
    }

    if(!IsApproximately(bounds.BottomRight().y, floor(bounds.BottomRight().y)))
    {
        ++bottomRight.y;
    }

    bottomRight = bottomRight.Min(Vector2(_obstacleGrid.Cols(), _obstacleGrid.Rows()));

    for(int i = topLeft.y; i < bottomRight.y; ++i)
    {
        for(int j = topLeft.x; j < bottomRight.x; ++j)
        {
            ++_obstacleGrid[i][j].count;
        }
    }
}

void PathFinderService::RequestFlowField(Vector2 start, Vector2 end, Entity* owner)
{
    PathRequest request;
    request.start = start;
    request.startCell = PixelToCellCoordinate(start);
    request.end = end;
    request.endCell = PixelToCellCoordinate(end);
    request.owner = owner;

    //Log("End cell: %f, %f\n", end.x, end.y);

    // FIXME: workaround for when ending in a solid cell
    while(_obstacleGrid[request.endCell].count != 0)
    {
        ++request.endCell.x;
    }

    _requestQueue.push(request);
}

void PathFinderService::ReceiveEvent(const IEntityEvent& ev)
{
    if(ev.Is<UpdateEvent>())
    {
        CalculatePaths();
    }
    else if(auto renderEvent = ev.Is<RenderEvent>())
    {
        //Visualize(renderEvent->renderer);
    }
}

void PathFinderService::CalculatePaths()
{
    int calculationCount = 0;

    while(calculationCount < MaxGridCalculationsPerTick)
    {
        if(_requestQueue.empty())
        {
            return;
        }

        auto& request = _requestQueue.front();

        Entity* owner;
        if(!request.owner.TryGetValue(owner))
        {
            _requestQueue.pop();
            continue;
        }

        {
            for (auto pathFollower : pathFollowers)
            {
                if (pathFollower->owner == owner) continue;
                ++_obstacleGrid[scene->isometricSettings.WorldToIntegerTile(pathFollower->owner->Center())].count;
            }
        }

        if(request.status == PathRequestStatus::NotStarted)
        {
            WorkQueue emptyQueue;
            std::swap(emptyQueue, _workQueue);

            _fieldInProgress = std::make_shared<FlowField>(_obstacleGrid.Rows(), _obstacleGrid.Cols(), request.end);
            _fieldInProgress->pathFinder = this;

            _workQueue.push(request.endCell);
            _fieldInProgress->grid[request.endCell].alreadyVisited = true;

            request.status = PathRequestStatus::InProgress;
        }

        while(calculationCount < MaxGridCalculationsPerTick && !_workQueue.empty())
        {
            auto cell = _workQueue.front();
            _workQueue.pop();

            const Vector2 directions[] =
            {
                Vector2(0, 1),
                Vector2(0, -1),
                Vector2(1, 0),
                Vector2(-1, 0),
                Vector2(1, 1),      // For ramps
            };

            for(auto direction : directions)
            {
                EnqueueCellIfValid(cell + direction, cell);
                ++calculationCount;
            }
        }

        if(_workQueue.empty())
        {
            owner->SendEvent(FlowFieldReadyEvent(_fieldInProgress));
            _fieldInProgress = nullptr;
            _requestQueue.pop();
        }

        {
            for (auto pathFollower : pathFollowers)
            {
                if (pathFollower->owner == owner) continue;
                --_obstacleGrid[scene->isometricSettings.WorldToIntegerTile(pathFollower->owner->Center())].count;
            }
        }
    }
}

void PathFinderService::Visualize(Renderer* renderer)
{
    for(int i = 0; i < _obstacleGrid.Rows(); ++i)
    {
        for(int j = 0; j < _obstacleGrid.Cols(); ++j)
        {
            Vector2 points[4] =
            {
                scene->isometricSettings.TileToWorld(Vector2(j, i)),
                scene->isometricSettings.TileToWorld(Vector2(j + 1, i)),
                scene->isometricSettings.TileToWorld(Vector2(j + 1, i + 1)),
                scene->isometricSettings.TileToWorld(Vector2(j, i + 1)),
            };

            ObstacleEdgeFlags flags[4] =
            {
                ObstacleEdgeFlags::NorthBlocked,
                ObstacleEdgeFlags::EastBlocked,
                ObstacleEdgeFlags::SouthBlocked,
                ObstacleEdgeFlags::WestBlocked
            };

            for (int k = 0; k < 4; ++k)
            {
                Vector2 offset(0, 0);

                if (_obstacleGrid[i][j].flags.HasFlag(flags[k]) || true)
                    renderer->RenderLine(points[k] + offset, points[(k + 1) % 4] + offset, Color::Red(), -1);
            }

            if (_obstacleGrid[i][j].count != 0)
            {
                Color colors[3] = {Color::Green(), Color::Yellow(), Color::Red()};

                Vector2 size(4, 4);
                renderer->RenderRectangle(
                    Rectangle(scene->isometricSettings.TileToWorld(Vector2(j, i) + Vector2(0.5)) - size / 2, size),
                    colors[(int)scene->isometricSettings.terrain[Vector2(j, i)].height],
                    -1);
            }
        }
    }
}

Vector2 PathFinderService::PixelToCellCoordinate(Vector2 position) const
{
    return position.Floor().AsVectorOfType<float>();
}

struct RampInfo
{
    RampInfo(RampType rampType, Vector2 rampDirection)
        : rampType(rampType),
        rampDirection(rampDirection)
    {

    }

    Vector2 rampDirection;
    RampType rampType;
};

bool TryGetRampOffset(const IsometricSettings& isometricSettings, const RampInfo& ramp, Vector2 from, Vector2 to, Vector2& outOffset)
{
    bool directionIsBlocked = isometricSettings.terrain[from].height != isometricSettings.terrain[to].height;
    
    if (isometricSettings.terrain[to].rampType == ramp.rampType)
    {
        if (directionIsBlocked && (to - from == ramp.rampDirection))
        {
            outOffset = Vector2(-1, -1);
            return true;
        }
        else if (!directionIsBlocked && to - from == Vector2(1, 1))
        {
            outOffset = -ramp.rampDirection;
            return true;
        }
    }

    return false;
}

static const RampInfo g_westRamp(RampType::West, Vector2(-1, 0));
static const RampInfo g_northRamp(RampType::North, Vector2(0, -1));
static const RampInfo g_invalidRamp(RampType::None, Vector2(100, 100));

static const RampInfo* GetRampInfo(RampType type)
{
    if (type == RampType::North) return &g_northRamp;
    if (type == RampType::West) return &g_westRamp;
    return &g_invalidRamp;
}


void PathFinderService::EnqueueCellIfValid(Vector2 cell, Vector2 from)
{
    bool directionIsBlocked = scene->isometricSettings.terrain[from].height != scene->isometricSettings.terrain[cell].height;

    {
        auto& below = scene->isometricSettings.terrain[from + Vector2(1)];
        auto dir = cell - from;
        if (below.rampType != RampType::None)
        {
            auto info = GetRampInfo(below.rampType);
            if (dir != info->rampDirection && dir != -info->rampDirection && dir != Vector2(1, 1))
            {
                return;
            }
        }
    }

    {
        auto& below = scene->isometricSettings.terrain[cell + Vector2(1)];
        auto dir = cell - from;
        if (below.rampType != RampType::None)
        {
            auto info = GetRampInfo(below.rampType);
            if (dir != info->rampDirection && dir != -info->rampDirection)
            {
                return;
            }
        }
    }

    Vector2 rampOffset;
    if (TryGetRampOffset(scene->isometricSettings, g_westRamp, from, cell, rampOffset))
    {
        directionIsBlocked = false;
        cell = cell + rampOffset;
    }
    else if (TryGetRampOffset(scene->isometricSettings, g_northRamp, from, cell, rampOffset))
    {
        directionIsBlocked = false;
        cell = cell + rampOffset;
    }
    else if (cell - from == Vector2(1, 1))
    {
        return;
    }

    if(cell.x >= 0
        && cell.x < _obstacleGrid.Cols()
        && cell.y >= 0
        && cell.y < _obstacleGrid.Rows()
        && _obstacleGrid[cell].count == 0
        && !_fieldInProgress->grid[cell].alreadyVisited
        && !directionIsBlocked)
    {
        _workQueue.push(cell);
        _fieldInProgress->grid[cell].alreadyVisited = true;
        _fieldInProgress->grid[cell].dir = from - cell;
    }
}

void PathFinderService::RemoveObstacle(const Rectangle &bounds)
{
    auto topLeft = PixelToCellCoordinate(bounds.TopLeft()).Max({ 0, 0 });
    auto bottomRight = PixelToCellCoordinate(bounds.BottomRight());

    if(!IsApproximately(bounds.BottomRight().x, floor(bounds.BottomRight().x)))
    {
        ++bottomRight.x;
    }

    if(!IsApproximately(bounds.BottomRight().y, floor(bounds.BottomRight().y)))
    {
        ++bottomRight.y;
    }

    bottomRight = bottomRight.Min(Vector2(_obstacleGrid.Cols(), _obstacleGrid.Rows()));

    for(int i = topLeft.y; i < bottomRight.y; ++i)
    {
        for(int j = topLeft.x; j < bottomRight.x; ++j)
        {
            --_obstacleGrid[i][j].count;
        }
    }
}

void PathFinderService::AddEdge(Vector2 from, Vector2 to)
{
    return;
}

void PathFinderService::RemoveEdge(Vector2 from, Vector2 to)
{
    return;
}
