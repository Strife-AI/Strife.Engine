#include "ML.hpp"

#include "Math/BitMath.hpp"
#include "Math/Rectangle.hpp"
#include "Physics/ColliderHandle.hpp"
#include "Scene/Scene.hpp"

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
        | (uint64_t)type;
}

static int GetCompressedRectangleField(uint64_t value, int id)
{
    uint64_t mask = (1UL << bitsPerField) - 1;
    return value >> id * bitsPerField & mask;
}

static Vector2 PixelToCellCoordinate(Vector2 topLeft, Vector2 cellSize, Vector2 pixelCoordinate)
{
    return (pixelCoordinate - topLeft) / cellSize;
}

gsl::span<uint64_t> ReadGridSensorRectangles(
    Scene* scene,
    Vector2 center,
    Vector2 cellSize,
    int rows,
    int cols,
    SensorObjectDefinition* objectDefinition)
{
    Vector2 gridSizePixels = cellSize * Vector2(cols, rows);
    Rectangle gridPixelBounds(center - gridSizePixels / 2, gridSizePixels);

    const int maxRectangles = 8192;
    static ColliderHandle colliderPool[maxRectangles];
    static uint64_t outputStorage[maxRectangles];

    auto overlappingColliders = scene->FindOverlappingColliders(gridPixelBounds, colliderPool);

    int outputSize = Min((int)overlappingColliders.size(), maxRectangles);
    for(int i = 0; i < outputSize; ++i)
    {
        auto& collider = overlappingColliders[i];
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

        outputStorage[i] = PackCompressedRectangle(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y, type);
    }

    return gsl::span<uint64_t>(outputStorage, outputSize);
}

void DecompressGridSensorOutput(gsl::span<uint64_t> compressedRectangles, Grid<int>& outGrid, SensorObjectDefinition* objectDefinition)
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