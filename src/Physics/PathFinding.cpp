#include "PathFinding.hpp"
#include "Renderer.hpp"
#include "Components/PathFollowerComponent.hpp"

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

    if (_obstacleGrid[request.endCell].count != 0)
    {
        auto diff = request.startCell - request.endCell;
        Vector2 offset = Abs(diff.x) > Abs(diff.y)
            ? Vector2(Sign(diff.x), 0)
            : Vector2(0, Sign(diff.y));

        if (offset == Vector2(0))
        {
            return;
        }

        while(_obstacleGrid[request.endCell].count != 0)
        {
            request.endCell += offset;
        }

        request.end = request.endCell;
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
                ++_obstacleGrid[scene->isometricSettings.ScreenToIntegerTile(pathFollower->owner->Center())].count;
            }
        }

        if(request.status == PathRequestStatus::NotStarted)
        {
            std::queue<Vector2> emptyQueue;
            std::swap(emptyQueue, _workQueue);

            _fieldInProgress = std::make_shared<FlowField>(_obstacleGrid.Rows(), _obstacleGrid.Cols(), request.startCell, request.endCell, request.end);
            _fieldInProgress->pathFinder = this;

            _workQueue.push(request.endCell);
            _fieldInProgress->grid[request.endCell].alreadyVisited = true;

            request.status = PathRequestStatus::InProgress;
        }

        while(calculationCount < MaxGridCalculationsPerTick && !_workQueue.empty())
        {
            auto cell = _workQueue.front();
            _workQueue.pop();

            _fieldInProgress->grid[cell].hasLineOfSightToGoal = HasLineOfSight(_fieldInProgress.get(), cell, _fieldInProgress->endCell);

            const Vector2 directions[] =
            {
                Vector2(0, 1),
                Vector2(0, -1),
                Vector2(1, 0),
                Vector2(-1, 0)
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
                --_obstacleGrid[scene->isometricSettings.ScreenToIntegerTile(pathFollower->owner->Center())].count;
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
                scene->isometricSettings.TileToScreen(Vector2(j, i)),
                scene->isometricSettings.TileToScreen(Vector2(j + 1, i)),
                scene->isometricSettings.TileToScreen(Vector2(j + 1, i + 1)),
                scene->isometricSettings.TileToScreen(Vector2(j, i + 1)),
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
                if (_obstacleGrid[i][j].flags.HasFlag(flags[k]))
                    renderer->RenderLine(points[k], points[(k + 1) % 4], Color::Red(), -1);
            }

#if true
            if (_obstacleGrid[i][j].count != 0)
            {
                Color colors[3] = {Color::Green(), Color::Yellow(), Color::Red()};

                Vector2 size(4, 4);
                renderer->RenderRectangle(
                    Rectangle(scene->isometricSettings.TileToScreen(Vector2(j, i) + Vector2(0.5)) - size / 2, size),
                    colors[(int)_obstacleGrid[Vector2(j, i)].height],
                    -1);
            }
#endif
        }
    }
}

Vector2 PathFinderService::PixelToCellCoordinate(Vector2 position) const
{
    return position.Floor().AsVectorOfType<float>();
}

ObstacleEdgeFlags GetBlockedDirection(Vector2 from, Vector2 to)
{
    auto diff = to - from;
    if (diff == Vector2(1, 0)) return ObstacleEdgeFlags::EastBlocked;
    if (diff == Vector2(-1, 0)) return ObstacleEdgeFlags::WestBlocked;
    if (diff == Vector2(0, -1)) return ObstacleEdgeFlags::NorthBlocked;
    if (diff == Vector2(0, 1)) return ObstacleEdgeFlags::SouthBlocked;

    return ObstacleEdgeFlags();
}

void PathFinderService::EnqueueCellIfValid(Vector2 cell, Vector2 from)
{
    bool isBlocked = IsBlocked(from, cell) && cell != _fieldInProgress->startCell;

    if(cell.x >= 0
        && cell.x < _obstacleGrid.Cols()
        && cell.y >= 0
        && cell.y < _obstacleGrid.Rows()
        && !_fieldInProgress->grid[cell].alreadyVisited
        && !isBlocked)
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
    _obstacleGrid[from].flags.SetFlag(GetBlockedDirection(from, to));
    _obstacleGrid[to].flags.SetFlag(GetBlockedDirection(to, from));
}

void PathFinderService::RemoveEdge(Vector2 from, Vector2 to)
{
    _obstacleGrid[from].flags.ResetFlag(GetBlockedDirection(from, to));
    _obstacleGrid[to].flags.ResetFlag(GetBlockedDirection(to, from));
}

bool PathFinderService::IsBlocked(Vector2 from, Vector2 to)
{
    return _obstacleGrid[from].flags.HasFlag(GetBlockedDirection(from, to))
        || _obstacleGrid[to].flags.HasFlag(GetBlockedDirection(to, from))
        || _obstacleGrid[to].count != 0;
}

bool PathFinderService::HasLineOfSight(FlowField* field, Vector2 at, Vector2 endCell)
{
    endCell = endCell.Floor().AsVectorOfType<float>();
    if (at.x == 0 || at.x == field->grid.Cols() || at.y == 0 || at.y == field->grid.Rows())
    {
        return false;
    }

    if (at == endCell)
    {
        return true;
    }
    else
    {
        auto diff = endCell - at;
        auto distX = Abs(diff.x);
        auto distY = Abs(diff.y);
        auto stepX = Sign(diff.x);
        auto stepY = Sign(diff.y);

        bool hasLineOfSight = false;

        if (distX >= distY)
        {
            if (!IsBlocked(at, at + Vector2(stepX, 0)))
            {
                hasLineOfSight |= field->grid[at + Vector2(stepX, 0)].hasLineOfSightToGoal;
            }
        }

        if (distY >= distX)
        {
            if (!IsBlocked(at, at + Vector2(0, stepY)))
            {
                hasLineOfSight |= field->grid[at + Vector2(0, stepY)].hasLineOfSightToGoal;
            }
        }

        if (distX > 0 && distY > 0)
        {
            if (!field->grid[at + Vector2(stepX, stepY)].hasLineOfSightToGoal)
            {
                hasLineOfSight = false;
            }
            else if (distX == distY)
            {
                if (IsBlocked(at, at + Vector2(stepX, 0)) || IsBlocked(at, at + Vector2(0, stepY)))
                {
                    hasLineOfSight = false;
                }
            }
        }

        return hasLineOfSight;
    }
}
