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
    auto cell = (position / tileSize)
        .Floor()
        .AsVectorOfType<float>()
        .Clamp({ 0.0f, 0.0f }, grid.Dimensions() - Vector2(1));

    return FlowDirectionToVector2(grid[cell].direction);
}

Vector2 FlowField::GetFilteredFlowDirection(Vector2 position)
{
    Rectangle bounds(position, tileSize);

    auto topLeft = GetFlowDirectionAtCell(bounds.TopLeft());
    auto topRight = GetFlowDirectionAtCell(bounds.TopRight());
    auto bottomLeft = GetFlowDirectionAtCell(bounds.BottomLeft());
    auto bottomRight = GetFlowDirectionAtCell(bounds.BottomRight());

    auto tileTopLeft = (position / tileSize).Floor().AsVectorOfType<float>() * tileSize;

    auto t = (position - tileTopLeft) / tileSize;



    return Lerp(
        Lerp(topLeft, topRight, t.x).Normalize(),
        Lerp(bottomLeft, bottomRight, t.x).Normalize(),
        t.y).Normalize();
}

PathFinderService::PathFinderService(int rows, int cols, Vector2 tileSize)
    : _obstacleGrid(rows, cols),
    _tileSize(tileSize)
{
    _obstacleGrid.FillWithZero();
}

void PathFinderService::AddObstacle(const Rectangle& bounds)
{
    auto topLeft = PixelToCellCoordinate(bounds.TopLeft()).Max({ 0, 0 });
    auto bottomRight = PixelToCellCoordinate(bounds.BottomRight());

    if((int)bounds.BottomRight().x % (int)_tileSize.x != 0)
    {
        ++bottomRight.x;
    }

    if ((int)bounds.BottomRight().y % (int)_tileSize.y != 0)
    {
        ++bottomRight.y;
    }

    bottomRight = bottomRight.Min(Vector2(_obstacleGrid.Cols(), _obstacleGrid.Rows()));

    for(int i = topLeft.y; i < bottomRight.y; ++i)
    {
        for(int j = topLeft.x; j < bottomRight.x; ++j)
        {
            ++_obstacleGrid[i][j];
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
    while(_obstacleGrid[request.endCell] != 0)
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

            _fieldInProgress = std::make_shared<FlowField>(_obstacleGrid.Rows(), _obstacleGrid.Cols(), request.end, _tileSize);

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
                EnqueueCellIfValid(cell + FlowDirectionToVector2(direction), direction);
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
            auto center = Vector2(j, i) * _tileSize + _tileSize / 2;
            auto dotSize = Vector2(4, 4);

            if (_obstacleGrid[i][j] != 0)
            {
                renderer->RenderRectangle(Rectangle(center - dotSize / 2, dotSize), Color::Red(), -1);
            }
        }
    }
}

Vector2 PathFinderService::PixelToCellCoordinate(Vector2 position) const
{
    return (position / _tileSize).Floor().AsVectorOfType<float>();
}

void PathFinderService::EnqueueCellIfValid(Vector2 cell, FlowDirection fromDirection)
{
    if(cell.x >= 0
        && cell.x < _obstacleGrid.Cols()
        && cell.y >= 0
        && cell.y < _obstacleGrid.Rows()
        && _obstacleGrid[cell] == 0
        && _fieldInProgress->grid[cell].direction == FlowDirection::Unset)
    {
        _workQueue.push(cell);
        _fieldInProgress->grid[cell].direction = OppositeFlowDirection(fromDirection);
    }
}

void PathFinderService::RemoveObstacle(const Rectangle &bounds)
{
    auto topLeft = PixelToCellCoordinate(bounds.TopLeft()).Max({ 0, 0 });
    auto bottomRight = PixelToCellCoordinate(bounds.BottomRight());

    if((int)bounds.BottomRight().x % (int)_tileSize.x != 0)
    {
        ++bottomRight.x;
    }

    if ((int)bounds.BottomRight().y % (int)_tileSize.y != 0)
    {
        ++bottomRight.y;
    }

    bottomRight = bottomRight.Min(Vector2(_obstacleGrid.Cols(), _obstacleGrid.Rows()));

    for(int i = topLeft.y; i < bottomRight.y; ++i)
    {
        for(int j = topLeft.x; j < bottomRight.x; ++j)
        {
            --_obstacleGrid[i][j];
        }
    }
}
