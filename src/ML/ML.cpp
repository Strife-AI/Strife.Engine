#include "ML.hpp"

#include "Math/BitMath.hpp"
#include "Math/Rectangle.hpp"
#include "Physics/ColliderHandle.hpp"
#include "Scene/Scene.hpp"

static void ClampInRange(int& value)
{
    value = Clamp(value, 0, 4095);
}

static uint64_t PackCompressedRectangle(int x1, int y1, int x2, int y2, int type)
{
    ClampInRange(x1);
    ClampInRange(y1);
    ClampInRange(x2);
    ClampInRange(y2);
    ClampInRange(type);

    int bitsPerElement = 12;

    return (uint64_t)x1 << 0 * bitsPerElement
        | (uint64_t)y1 << 1 * bitsPerElement
        | (uint64_t)x2 << 2 * bitsPerElement
        | (uint64_t)y2 << 3 * bitsPerElement
        | (uint64_t)type;
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
    std::shared_ptr<SensorObjectDefinition>& objectDefinition)
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

        auto object = objectDefinition->objectByType.find(collider.OwningEntity()->type.key);
        int type = object != objectDefinition->objectByType.end()
            ? object->second.id
            : 0;

        outputStorage[i] = PackCompressedRectangle(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y, type);
    }

    return gsl::span<uint64_t>(outputStorage, outputSize);
}