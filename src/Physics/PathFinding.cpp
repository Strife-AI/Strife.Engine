#include "PathFinding.hpp"

#include "Renderer.hpp"

Vector2 FlowDirectionToVector2(FlowDirection direction)
{
    switch (direction)
    {
    case FlowDirection::Zero: return Vector2(0);
    case FlowDirection::North: return Vector2(0, -1);
    case FlowDirection::East: return Vector2(1, 0);
    case FlowDirection::South: return Vector2(0, 1);
    case FlowDirection::West: return Vector2(-1, 0);
    default: return Vector2(0);
    }
}

FlowDirection OppositeFlowDirection(FlowDirection direction)
{
    if(direction == FlowDirection::Unset || direction == FlowDirection::Zero)
    {
        return direction;
    }
    else
    {
        return (FlowDirection)(((int)direction + 2) % 4);
    }
}

Vector2 FlowField::GetFlowDirectionAtCell(Vector2 position)
{
    auto cell = position
        .Floor()
        .AsVectorOfType<float>()
        .Clamp({ 0.0f, 0.0f }, grid.Dimensions() - Vector2(1));

    return FlowDirectionToVector2(grid[cell].direction);
}

Vector2 FlowField::GetFilteredFlowDirection(Vector2 position)
{
    Rectangle bounds(position, Vector2(1));

    auto topLeft = GetFlowDirectionAtCell(bounds.TopLeft());
    auto topRight = GetFlowDirectionAtCell(bounds.TopRight());
    auto bottomLeft = GetFlowDirectionAtCell(bounds.BottomLeft());
    auto bottomRight = GetFlowDirectionAtCell(bounds.BottomRight());

    auto tileTopLeft = position.Floor().AsVectorOfType<float>();

    auto t = position - tileTopLeft;

    return Lerp(
        Lerp(topLeft, topRight, t.x).Normalize(),
        Lerp(bottomLeft, bottomRight, t.x).Normalize(),
        t.y).Normalize();
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

        if(request.status == PathRequestStatus::NotStarted)
        {
            WorkQueue emptyQueue;
            std::swap(emptyQueue, _workQueue);

            _fieldInProgress = std::make_shared<FlowField>(_obstacleGrid.Rows(), _obstacleGrid.Cols(), request.end);

            _workQueue.push(request.endCell);
            _fieldInProgress->grid[request.endCell].direction = FlowDirection::Zero;

            request.status = PathRequestStatus::InProgress;
        }

        while(calculationCount < MaxGridCalculationsPerTick && !_workQueue.empty())
        {
            auto cell = _workQueue.front();
            _workQueue.pop();

            const FlowDirection directions[] =
            {
                FlowDirection::North,
                FlowDirection::East,
                FlowDirection::West,
                FlowDirection::South,
            };

            for(auto direction : directions)
            {
                EnqueueCellIfValid(cell + FlowDirectionToVector2(direction), cell, direction);
                ++calculationCount;
            }
        }

        if(_workQueue.empty())
        {
            owner->SendEvent(FlowFieldReadyEvent(_fieldInProgress));
            _fieldInProgress = nullptr;
            _requestQueue.pop();
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
        }
    }
}

Vector2 PathFinderService::PixelToCellCoordinate(Vector2 position) const
{
    return position.Floor().AsVectorOfType<float>();
}

static ObstacleEdgeFlags GetDirection(Vector2 from, Vector2 to)
{
    Vector2 diff = to - from;
    ObstacleEdgeFlags dir = ObstacleEdgeFlags::NorthBlocked;
    if (diff == Vector2(1, 0)) return ObstacleEdgeFlags::EastBlocked;
    if (diff == Vector2(-1, 0)) return ObstacleEdgeFlags::WestBlocked;
    if (diff == Vector2(0, -1)) return ObstacleEdgeFlags::NorthBlocked;
    if (diff == Vector2(0, 1)) return ObstacleEdgeFlags::SouthBlocked;

    FatalError("Bad dir %f %f\n", diff.x, diff.y);
    return dir;
}

static ObstacleEdgeFlags ReverseDirection(ObstacleEdgeFlags dir)
{
    switch (dir)
    {
    case ObstacleEdgeFlags::EastBlocked: return ObstacleEdgeFlags::WestBlocked;
    case ObstacleEdgeFlags::WestBlocked: return ObstacleEdgeFlags::EastBlocked;
    case ObstacleEdgeFlags::NorthBlocked: return ObstacleEdgeFlags::SouthBlocked;
    case ObstacleEdgeFlags::SouthBlocked: return ObstacleEdgeFlags::NorthBlocked;
    }

    return ObstacleEdgeFlags::NorthBlocked;
}

void PathFinderService::EnqueueCellIfValid(Vector2 cell, Vector2 from, FlowDirection fromDirection)
{
    auto dir = GetDirection(from, cell);
    bool directionIsBlocked = _obstacleGrid[from].flags.HasFlag(dir);

    if(cell.x >= 0
        && cell.x < _obstacleGrid.Cols()
        && cell.y >= 0
        && cell.y < _obstacleGrid.Rows()
        && _obstacleGrid[cell].count == 0
        && _fieldInProgress->grid[cell].direction == FlowDirection::Unset
        && !directionIsBlocked)
    {
        _workQueue.push(cell);
        _fieldInProgress->grid[cell].direction = OppositeFlowDirection(fromDirection);
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
    // TODO: bounds check on from and to
    auto dir = GetDirection(from, to);
    _obstacleGrid[from].flags.SetFlag(dir);
    _obstacleGrid[to].flags.SetFlag(ReverseDirection(dir));
}

void PathFinderService::RemoveEdge(Vector2 from, Vector2 to)
{
    // TODO: bounds check on from and to
    auto dir = GetDirection(from, to);
    _obstacleGrid[from].flags.ResetFlag(dir);
    _obstacleGrid[to].flags.ResetFlag(ReverseDirection(dir));
}
