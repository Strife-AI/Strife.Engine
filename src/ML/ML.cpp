#include "ML.hpp"


#include "Renderer.hpp"
#include "Math/BitMath.hpp"
#include "Math/Rectangle.hpp"
#include "Physics/ColliderHandle.hpp"
#include "Scene/Scene.hpp"

std::shared_ptr<SensorObjectDefinition> NeuralNetworkManager::_sensorObjectDefinition;

int bitsPerField = 12;

static void ClampInRange(int& value)
{
    value = Clamp(value, 0, 1 << bitsPerField - 1);
}

static uint64_t PackCompressedRectangle(int x1, int y1, int x2, int y2, int type)
{
    ClampInRange(x1);
    ClampInRange(y1);
    ClampInRange(x2);
    ClampInRange(y2);
    ClampInRange(type);

    return (uint64_t)x1 << 0 * bitsPerField
        | (uint64_t)y1 << 1 * bitsPerField
        | (uint64_t)x2 << 2 * bitsPerField
        | (uint64_t)y2 << 3 * bitsPerField
        | (uint64_t)type << 4 * bitsPerField;
}

static int GetCompressedRectangleField(uint64_t value, int id)
{
    uint64_t mask = (1UL << bitsPerField) - 1;
    return value >> id * bitsPerField & mask;
}

static Vector2 PixelToCellCoordinate(Vector2 topLeft, Vector2 cellSize, Vector2 pixelCoordinate)
{
    return ((pixelCoordinate - topLeft) / cellSize).Floor().AsVectorOfType<float>();
}

gsl::span<uint64_t> ReadGridSensorRectangles(
    Scene* scene,
    Vector2 center,
    Vector2 cellSize,
    int rows,
    int cols,
    SensorObjectDefinition* objectDefinition,
    Entity* self)
{
    Vector2 gridSizePixels = cellSize * Vector2(cols, rows);
    Vector2 topLeft =  ((center - gridSizePixels / 2) / cellSize).Round() * cellSize;

    Rectangle gridPixelBounds(topLeft, gridSizePixels);

    const int maxRectangles = 8192;
    static ColliderHandle colliderPool[maxRectangles];
    static uint64_t outputStorage[maxRectangles];

    auto overlappingColliders = scene->FindOverlappingColliders(gridPixelBounds, colliderPool);

    int outputSize = 0;
    for(int i = 0; i < Min((int)overlappingColliders.size(), maxRectangles); ++i)
    {
        auto& collider = overlappingColliders[i];

        if(collider.OwningEntity() == self)
        {
            continue;
        }

        auto colliderBounds = collider.Bounds();

        auto topLeft = PixelToCellCoordinate(gridPixelBounds.TopLeft(), cellSize, colliderBounds.TopLeft()).Max({ 0, 0 });
        auto bottomRight = PixelToCellCoordinate(gridPixelBounds.TopLeft(), cellSize, colliderBounds.BottomRight());

        if ((int)colliderBounds.BottomRight().x % (int)cellSize.x != 0)
        {
            ++bottomRight.x;
        }

        if ((int)colliderBounds.BottomRight().y % (int)cellSize.y != 0)
        {
            ++bottomRight.y;
        }

        int type = objectDefinition->GetEntitySensorObject(collider.OwningEntity()->type.key).id;

        outputStorage[outputSize++] = PackCompressedRectangle(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y, type);
    }

    return gsl::span<uint64_t>(outputStorage, outputSize);
}

void DecompressGridSensorOutput(gsl::span<const uint64_t> compressedRectangles, Grid<int>& outGrid, SensorObjectDefinition* objectDefinition)
{
    outGrid.FillWithZero();

    for(auto compressedRectangle : compressedRectangles)
    {
        int x1 = Clamp(GetCompressedRectangleField(compressedRectangle, 0), 0, outGrid.Cols());
        int y1 = Clamp(GetCompressedRectangleField(compressedRectangle, 1), 0, outGrid.Rows());
        int x2 = Clamp(GetCompressedRectangleField(compressedRectangle, 2), 0, outGrid.Cols());
        int y2 = Clamp(GetCompressedRectangleField(compressedRectangle, 3), 0, outGrid.Rows());
        int type = Clamp(GetCompressedRectangleField(compressedRectangle, 4), 0, objectDefinition->maxId);

        if(objectDefinition->objectById.find(type) == objectDefinition->objectById.end())
        {
            // Missing type, replace with empty
            type = 0;
        }

        for(int y = y1; y < y2; ++y)
        {
            for(int x = x1; x < x2; ++x)
            {
                if(objectDefinition->objectById[type].priority > objectDefinition->objectById[outGrid[y][x]].priority)
                {
                    outGrid[y][x] = type;
                }
            }
        }
    }
}

void RenderGridSensorOutput(Grid<int>& grid, Vector2 center, Vector2 cellSize, SensorObjectDefinition* objectDefinition, Renderer* renderer, float depth)
{
    Vector2 gridSize = cellSize * Vector2(grid.Cols(), grid.Rows());
    Vector2 gridTopLeft = ((center - gridSize / 2) / cellSize).Round() * cellSize;
    Vector2 gridBottomRight = gridTopLeft + gridSize;

    for(int i = 0; i < grid.Rows(); ++i)
    {
        for(int j = 0; j < grid.Cols(); ++j)
        {
            Vector2 cellTopLeft = gridTopLeft + Vector2(j, i) * cellSize;

            auto it = objectDefinition->objectById.find(grid[i][j]);
            auto& object = it != objectDefinition->objectById.end()
                ? it->second
                : objectDefinition->objectById[0];

            if (object.id != 0)
            {
                renderer->RenderRectangle(Rectangle(cellTopLeft, cellSize), object.color, depth, 0);
            }
        }
    }

    for(int i = 0; i < grid.Rows() + 1; ++i)
    {
        float y = gridTopLeft.y + i * cellSize.y;
        renderer->RenderLine(Vector2(gridTopLeft.x, y), Vector2(gridBottomRight.x, y), Color::Gray(), depth);
    }

    for(int i = 0; i < grid.Cols() + 1; ++i)
    {
        float x = gridTopLeft.x + i * cellSize.x;
        renderer->RenderLine(Vector2(x, gridTopLeft.y), Vector2(x, gridBottomRight.y), Color::Gray(), 0);
    }
}