#include "ML.hpp"


#include "Renderer.hpp"
#include "Math/BitMath.hpp"
#include "Math/Rectangle.hpp"
#include "Physics/ColliderHandle.hpp"
#include "Scene/Scene.hpp"
#include "box2d/b2_edge_shape.h"
#include "box2d/b2_circle_shape.h"
#include "box2d/b2_polygon_shape.h"

std::shared_ptr<SensorObjectDefinition> NeuralNetworkManager::_sensorObjectDefinition;

int bitsPerField = 12;

static void ClampInRange(int& value)
{
    value = Clamp(value, 0, 1 << (bitsPerField - 1));
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

Rectangle GetIsometricBounds(b2Fixture* fixture, IsometricSettings* isometric, Vector2 tileSize)
{
    auto shape = fixture->GetShape();
    auto body = fixture->GetBody();

    switch (shape->GetType())
    {
    case b2Shape::e_polygon:
    {
        auto polygon = static_cast<const b2PolygonShape*>(shape);
        Vector2 min = Vector2(1000000, 1000000);
        Vector2 max = -min;

        for (int i = 0; i < polygon->m_count; ++i)
        {
            auto v = body->GetWorldPoint(polygon->m_vertices[i]);
            auto v2 = isometric->WorldToTile(Scene::Box2DToPixel(v), tileSize);

            min = min.Min(v2);
            max = max.Max(v2);
        }

        return Rectangle(min, max - min);
    }

    case b2Shape::e_edge:
    {
        auto edge = static_cast<const b2EdgeShape*>(shape);
        auto a = Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex1));
        auto b = Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex2));

        return Rectangle::FromPoints(
            isometric->WorldToTile(a, tileSize),
            isometric->WorldToTile(b, tileSize));
    }


    case b2Shape::e_circle:
    {
        auto circle = static_cast<const b2CircleShape*>(shape);
        auto center = Scene::Box2DToPixel(body->GetWorldPoint(circle->m_p));
        float radius = circle->m_radius * Scene::Box2DToPixelsRatio.x;
        float diagonal = radius * sqrt(2) / 2;

        auto p1 = isometric->WorldToTile(center - Vector2(diagonal), tileSize);
        auto p2 = isometric->WorldToTile(center + Vector2(diagonal), tileSize);

        return Rectangle::FromPoints(p1, p2);

    }

    default:
        FatalError("Unsupported fixture type: %d", (int)shape->GetType());
    }
}

gsl::span<uint64_t> ReadGridSensorRectanglesIsometric(
    Scene* scene,
    Vector2 center,
    Vector2 cellSize,
    int rows,
    int cols,
    SensorObjectDefinition* objectDefinition,
    Entity* self)
{
    center = IsometricSettings::TileToWorldCustomSize(IsometricSettings::WorldToTile(center, cellSize).Floor().AsVectorOfType<float>(), cellSize);

    Vector2 gridSizePixels = cellSize * Vector2(cols, rows);
    Vector2 topLeft =  (center - gridSizePixels / 2).RoundTo(cellSize);

    Rectangle gridPixelBounds(topLeft, gridSizePixels);
    auto topLeftTile = scene->isometricSettings.WorldToTile(center - cellSize.YVector() * rows / 2, cellSize);

    const int maxRectangles = 8192;
    static ColliderHandle colliderPool[maxRectangles];
    static uint64_t outputStorage[maxRectangles];

    auto overlappingColliders = scene->FindOverlappingColliders(gridPixelBounds, colliderPool);

    int outputSize = 0;
    for(int i = 0; i < Min((int)overlappingColliders.size(), maxRectangles); ++i)
    {
        auto& collider = overlappingColliders[i];

        if (collider.OwningEntity() == self)
        {
            continue;
        }

        auto& definition = objectDefinition->GetEntitySensorObject(collider.OwningEntity()->type.key);
        if (collider.IsTrigger() && !definition.allowTriggers)
        {
            continue;
        }

        auto colliderBounds = GetIsometricBounds(collider.GetFixture(), &scene->isometricSettings, cellSize);
        auto min = colliderBounds.TopLeft();
        auto max = colliderBounds.BottomRight();
        auto topLeft = (min - topLeftTile).Floor().AsVectorOfType<float>();
        auto size = max - min;
        auto sizeInt = max.Floor() - min.Floor();

        if (floor(size.x) != size.x) ++sizeInt.x;
        if (floor(size.y) != size.y) ++sizeInt.y;

        auto bottomRight = topLeft + sizeInt.AsVectorOfType<float>();

        int type = definition.id;

        outputStorage[outputSize++] = PackCompressedRectangle(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y, type);
    }

    return gsl::span<uint64_t>(outputStorage, outputSize);
}

void DecompressGridSensorOutput(gsl::span<const uint64_t> compressedRectangles, Grid<uint64_t>& outGrid, SensorObjectDefinition* objectDefinition)
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

void RenderGridSensorOutput(Grid<uint64_t>& grid, Vector2 center, Vector2 cellSize, SensorObjectDefinition* objectDefinition, Renderer* renderer, float depth)
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
                renderer->RenderRectangle(Rectangle(cellTopLeft, cellSize), Color(object.color.r, object.color.g, object.color.b, 128), -1, 0);
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

Vector2 Transform(Vector2 input)
{
    return Vector2(input.x + input.y, (input.y - input.x) / 2);
}

void RenderGridSensorOutputIsometric(Grid<uint64_t>& grid, Vector2 center, Vector2 cellSize, SensorObjectDefinition* objectDefinition, Renderer* renderer, float depth)
{
    Vector2 gridSize = cellSize * Vector2(grid.Cols(), grid.Rows());
    Vector2 gridTopLeft = ((center - gridSize / 2) / cellSize).Round() * cellSize;
    Vector2 gridBottomRight = gridTopLeft + gridSize;

    Vector2 gridSizePixels = cellSize * grid.Dimensions();
    Vector2 topLeft = IsometricSettings::TileToWorldCustomSize(IsometricSettings::WorldToTile(center, cellSize).Floor().AsVectorOfType<float>(), cellSize);

    auto transform = [=](Vector2 tile)
    {
        return IsometricSettings::TileToWorldCustomSize(tile - grid.Dimensions() / 2, cellSize) + topLeft;
    };

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
                auto color = Color(object.color.r, object.color.g, object.color.b, 128);

                auto v = Vector2(j, i);

                Vector2 points[4] =
                {
                    transform(v),
                    transform(v + Vector2(1, 0)),
                    transform(v + Vector2(1, 1)),
                    transform(v + Vector2(0, 1))
                };

                renderer->GetSpriteBatcher()->RenderSolidPolygon(points, 4, -1, color);
            }
        }
    }

    for(int i = 0; i < grid.Rows() + 1; ++i)
    {
        renderer->RenderLine(transform(Vector2(0, i)), transform(Vector2(grid.Cols(), i)), Color::Black(), depth);
    }

    for(int i = 0; i < grid.Cols() + 1; ++i)
    {
        renderer->RenderLine(transform(Vector2(i, 0)), transform(Vector2(i, grid.Rows())), Color::Black(), depth);
    }
}