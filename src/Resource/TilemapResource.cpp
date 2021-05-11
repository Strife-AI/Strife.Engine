#include <System/BinaryStreamWriter.hpp>
#include <System/TileMapSerialization.hpp>
#include <System/ResourceFileWriter.hpp>
#include <Container/Grid.hpp>
#include <SDL.h>
#include <SDL_image.h>
#include "TilemapResource.hpp"

#include "tmxlite/Map.hpp"
#include "tmxlite/TileLayer.hpp"
#include "SpriteResource.hpp"
#include "ResourceSettings.hpp"

struct Coord
{
    Coord(int position_, bool relative_ = false)
        : position(position_),
          relative(relative_)
    {

    }

    int position;
    bool relative;
};

struct GridLayout
{
    std::vector<Coord> columns;
    std::vector<Coord> rows;
};

const int EdgePadding = 4;

std::string GetTmxPropertyValue(const tmx::Property& property)
{
    std::string propertyValue;

    switch (property.getType())
    {
    case tmx::Property::Type::String:
        propertyValue = property.getStringValue();
        break;

    case tmx::Property::Type::Boolean:
        propertyValue = property.getBoolValue() ? "true" : "false";
        break;

    case tmx::Property::Type::Int:
        propertyValue = std::to_string(property.getIntValue());
        break;

    case tmx::Property::Type::Float:
        propertyValue = std::to_string(property.getFloatValue());
        break;

    case tmx::Property::Type::Undef:
    default:
        propertyValue = "UNDEF";
        break;
    }

    return propertyValue;
}

static Uint32 getpixel(SDL_Surface* surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16*)p;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32*)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

static void putpixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16*)p = pixel;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        }
        else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32*)p = pixel;
        break;
    }
}

static auto DetectFormatFromSdlSurface(SDL_Surface* surface)
{
    auto format = surface->format;
    return SDL_MasksToPixelFormatEnum(format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);
}

static SDL_Surface* ConvertToRGBA(SDL_Surface* surface)
{
    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
    return SDL_ConvertSurface(surface, format, 0);
}

StringId AddEdgePadding(SDL_Surface* rawSurface, const std::string& resourceName, GridLayout& layout)
{
    auto format = DetectFormatFromSdlSurface(rawSurface);
    SDL_Surface* convertedSurface = nullptr;

    if (format != SDL_PIXELFORMAT_RGBA32)
    {
        convertedSurface = rawSurface = ConvertToRGBA(rawSurface);
    }

    for (auto& col : layout.columns)
    {
        if (col.relative) col.position += rawSurface->w;
    }

    for (auto& row : layout.rows)
    {
        if (row.relative) row.position += rawSurface->h;
    }

    Vector2 finalSize(
        rawSurface->w + (layout.columns.size() - 2) * 2 * EdgePadding,
        rawSurface->h + (layout.rows.size() - 2) * 2 * EdgePadding);

    SDL_Surface* result = SDL_CreateRGBSurfaceWithFormat(
        0,
        finalSize.x,
        finalSize.y,
        rawSurface->format->BitsPerPixel,
        rawSurface->format->format);

    for (int tileY = 0; tileY < layout.rows.size() - 1; ++tileY)
    {
        for (int tileX = 0; tileX < layout.columns.size() - 1; ++tileX)
        {
            Vector2 tileTopLeft(layout.columns[tileX].position, layout.rows[tileY].position);
            Vector2 tileBottomRight(layout.columns[tileX + 1].position - 1, layout.rows[tileY].position - 1);

            Vector2 tileSize(layout.columns[tileX + 1].position - layout.columns[tileX].position, layout.rows[tileY + 1].position - layout.rows[tileY].position);

            for (int i = -EdgePadding; i < tileSize.y + EdgePadding; ++i)
            {
                for (int j = -EdgePadding; j < tileSize.x + EdgePadding; ++j)
                {
                    auto edgePadding = Vector2(tileX, tileY) * 2 * EdgePadding;
                    auto resultPosition = (tileTopLeft + edgePadding + Vector2(j, i)).Clamp(
                        Vector2(0, 0),
                        finalSize - Vector2(1, 1));

                    auto sourcePosition = tileTopLeft + Vector2(j, i).Clamp(Vector2(0, 0), tileSize - Vector2(1, 1));

                    auto pixel = getpixel(rawSurface, sourcePosition.x, sourcePosition.y);
                    putpixel(result, resultPosition.x, resultPosition.y, pixel);
                }
            }
        }
    }

    auto resourceManager = ResourceManager::GetInstance();
    std::string file = (std::filesystem::path(resourceManager->GetBaseAssetPath())/"temp.png").string();

    int r = IMG_SavePNG(result, file.c_str());

    ResourceSettings settings;
    settings.resourceType = "sprite";
    settings.path = file.c_str();
    settings.resourceName = resourceName.c_str();

    resourceManager->LoadResourceFromFile(settings);

    //auto res = AddPng(writer, file, resourceName);

    SDL_FreeSurface(result);

    remove(file.c_str());

    if (convertedSurface != nullptr)
    {
        SDL_FreeSurface(convertedSurface);
    }

    return StringId(resourceName);
}

StringId AddTileSet(const std::string& fileName, const std::string& resourceName, Vector2 tileSize)
{
    if (ResourceManager::GetInstance()->GetResourceByName(resourceName.c_str()) != nullptr)
    {
        return StringId(resourceName);
    }

    GridLayout layout;

    auto rawSurface = IMG_Load(fileName.c_str());

    int x = 0;
    while (x <= rawSurface->w)
    {
        layout.columns.emplace_back(x);
        x += tileSize.x;
    }

    int y = 0;
    while (y <= rawSurface->h)
    {
        layout.rows.emplace_back(y);
        y += tileSize.y;
    }

    auto res = AddEdgePadding(rawSurface, resourceName, layout);
    SDL_FreeSurface(rawSurface);
    return res;
}

void LoadTileLayer(
    tmx::Map& map,
    tmx::TileLayer& layer,
    std::map<int, ImportTileProperties*>& tilePropertiesByTileId,
    MapSegmentDto* mapSegmentDto,
    Vector2 tileSize) {
    const auto& layerProperties = layer.getProperties();

    auto dimensions = map.getTileCount();
    int totalTiles = dimensions.x * dimensions.y;
    auto properties = new ImportTileProperties *[totalTiles];
    Grid<ImportTileProperties *> tileProperties(dimensions.y, dimensions.x, properties);
    auto tiles = layer.getTiles();

    for (int i = 0; i < static_cast<int>(dimensions.y); ++i)
    {
        for (int j = 0; j < static_cast<int>(dimensions.x); ++j)
        {
            int index = i * dimensions.x + j;

            auto tile = tiles[index];

            if (tile.ID != 0)
            {
                properties[index] = tilePropertiesByTileId[tile.ID];
            }
            else
            {
                properties[index] = nullptr;
            }
        }
    }

    if (layer.getVisible())
    {
        TileMapLayerDto layerDto;
        layerDto.tileSize = tileSize.AsVectorOfType<int>();
        layerDto.dimensions = Vector2i(dimensions.x, dimensions.y);

        for (int i = 0; i < (int)dimensions.x * dimensions.y; ++i)
        {
            int id = properties[i] != nullptr
                     ? properties[i]->serializationId
                     : -1;

            layerDto.tiles.push_back(id);
        }

        layerDto.layerName = StringId(layer.getName());

        mapSegmentDto->layers.push_back(std::move(layerDto));
    }

    if (layer.getName() == "Collision")
    {
        bool* alreadyUsedInColliderBools = new bool[totalTiles] {false};
        Grid<bool> alreadUsedInCollider(dimensions.y, dimensions.x, alreadyUsedInColliderBools);

        // Process polygonal colliders first
        for (int i = 0; i < alreadUsedInCollider.Rows(); ++i)
        {
            for (int j = 0; j < alreadUsedInCollider.Cols(); ++j)
            {
                // Check to see if the collider is already made into a shape.
                if (tileProperties[i][j] != nullptr && !tileProperties[i][j]->shape.IsEmpty())
                {
                    alreadUsedInCollider[i][j] = true;
                    auto newColliderShape = tileProperties[i][j]->shape;
                    for (auto& point : newColliderShape.points)
                    {
                        point.x += j * tileSize.x;
                        point.y += i * tileSize.y;
                    }

                    mapSegmentDto->polygonalColliders.push_back(newColliderShape);
                }
            }
        }

        // Generate rectangleColliders
        for (int i = 0; i < alreadUsedInCollider.Rows(); ++i)
        {
            for (int j = 0; j < alreadUsedInCollider.Cols(); ++j)
            {
                if (!alreadUsedInCollider[i][j] && tileProperties[i][j] != nullptr)
                {
                    int height = 1;

                    for (int y = i + 1; y < alreadUsedInCollider.Rows(); ++y)
                    {
                        if (!alreadUsedInCollider[y][j] && tileProperties[y][j] != nullptr)
                        {
                            ++height;
                        }
                        else
                        {
                            break;
                        }
                    }

                    int width = 1;
                    bool wholeColumnIsUnused = true;

                    for (int x = j + 1; x < alreadUsedInCollider.Cols() && wholeColumnIsUnused; ++x)
                    {
                        for (int y = i; y < i + height; ++y)
                        {
                            if (alreadUsedInCollider[y][x] || tileProperties[y][x] == nullptr)
                            {
                                wholeColumnIsUnused = false;
                                break;
                            }
                        }

                        if (wholeColumnIsUnused)
                        {
                            ++width;
                        }
                    }

                    for (int y = i; y < i + height; ++y)
                    {
                        for (int x = j; x < j + width; ++x)
                        {
                            alreadUsedInCollider[y][x] = true;
                        }
                    }

                    Vector2i topLeft = Vector2i(j, i) * tileSize.AsVectorOfType<int>();
                    Vector2i bottomRight = Vector2i(j + width, i + height) * tileSize.AsVectorOfType<int>();

                    mapSegmentDto->rectangleColliders.emplace_back(topLeft, bottomRight - topLeft);
                }
            }
        }

        delete[] alreadyUsedInColliderBools;
    }
}

void LoadObjectGroup(tmx::Map& map, tmx::ObjectGroup& layer, MapSegmentDto* mapSegmentDto)
{
    //TODO: Do checks the game relies on here.

    for (const auto& objectGroupProperty : layer.getProperties())
    {
        if (objectGroupProperty.getType() == tmx::Property::Type::Boolean
            && objectGroupProperty.getName() == "ignoreObjectGroup"
            && objectGroupProperty.getBoolValue())
        {
            return;
        }
    }
}

MapSegmentDto ProcessMap(const std::string& path)
{
    MapSegmentDto mapSegmentDto;

    tmx::Map map;
    if (!map.load(path))
    {
        FatalError("Couldn't load map segment %s", path.c_str());
    }

    std::map<int, ImportTileProperties*> tilePropertiesByTileId;
    std::map<int, int> localTileIdByGlobalId;

    int nextTileSerializationId = 0;

    std::optional<Vector2> tileSize;

    for (auto& tileSet : map.getTilesets())
    {
        auto imagePath = tileSet.getImagePath();
        auto resourceName = std::filesystem::path(imagePath).filename().string() + "-tileset";

        auto tileSetTileSize = Vector2(tileSet.getTileSize().x, tileSet.getTileSize().y);

        if (!tileSize.has_value())
        {
            tileSize = tileSetTileSize;
        }
        else
        {
            if (tileSetTileSize != tileSize)
            {
                FatalError("All tilesets within a map must have the same tile size");
            }
        }

        AddTileSet(imagePath, resourceName, tileSetTileSize);

        Vector2i tileSize = Vector2i(tileSet.getTileSize().x, tileSet.getTileSize().y);

        for (const auto& tile : tileSet.getTiles())
        {
            auto tileIndex = Vector2i(tile.imagePosition.x, tile.imagePosition.y) / tileSize;

            auto position = Vector2i(tile.imagePosition.x, tile.imagePosition.y) + tileIndex * Vector2i(EdgePadding * 2);
            Polygoni shape;

            if (!tile.objectGroup.getObjects().empty())
            {
                // Note: This only uses the first object, tiles can have multiple objects attached to them
                // This should be changed in the future, but for now it is as such
                auto colliderObject = tile.objectGroup.getObjects()[0];

                for (auto& point : colliderObject.getPoints())
                {
                    shape.points.emplace_back(point.x + colliderObject.getPosition().x ,
                        point.y + colliderObject.getPosition().y);
                }
            }

            std::map<std::string, std::string> properties;
            for (const auto& property : tile.properties)
            {
                properties[property.getName()] = GetTmxPropertyValue(property);
            }

            auto tileProperties = new ImportTileProperties(
                resourceName,
                tile.ID,
                Rectanglei(position, tileSize),
                shape,
                nextTileSerializationId++,
                properties);

            tilePropertiesByTileId[tile.ID + tileSet.getFirstGID()] = tileProperties;
            localTileIdByGlobalId[tile.ID + tileSet.getFirstGID()] = tile.ID;

            mapSegmentDto.tileProperties.push_back(tileProperties->ToTilePropertiesDto());
        }
    }

    for (const auto& layer : map.getLayers())
    {
        if (layer->getType() == tmx::Layer::Type::Tile)
        {
            auto tileLayer = layer->getLayerAs<tmx::TileLayer>();
            LoadTileLayer(map, tileLayer, tilePropertiesByTileId, &mapSegmentDto, tileSize.value());
        }
        else if (layer->getType() == tmx::Layer::Type::Object)
        {
            auto objectGroup = layer->getLayerAs<tmx::ObjectGroup>();
            LoadObjectGroup(map, objectGroup, &mapSegmentDto);
        }
    }

    for(const auto& prop : map.getProperties())
    {
        mapSegmentDto.properties[prop.getName()] = GetTmxPropertyValue(prop);
    }

    return mapSegmentDto;
}

void DtoToSegment(MapSegment* segment, MapSegmentDto& segmentDto)
{
    segment->colliders = std::move(segmentDto.rectangleColliders);
    segment->polygonColliders = std::move(segmentDto.polygonalColliders);

    // Load tiles
    for (auto& tilePropertiesDto : segmentDto.tileProperties)
    {
        auto tileBounds = tilePropertiesDto.bounds.As<float>();

        Sprite* tileSprite = &GetResource<SpriteResource>(tilePropertiesDto.spriteResource.c_str())->Get();
        segment->tileProperties.push_back(new TileProperties(
            Sprite(tileSprite->GetTexture(), tileBounds, tilePropertiesDto.bounds.As<float>()),
            0,
            tilePropertiesDto.properties));
    }

    // Load layers
    for (auto& layerDto : segmentDto.layers)
    {
        int totalTiles = layerDto.dimensions.x * layerDto.dimensions.y;
        TileProperties** tiles = new TileProperties * [totalTiles];

        for (int i = 0; i < totalTiles; ++i)
        {
            tiles[i] = layerDto.tiles[i] != -1
                       ? segment->tileProperties[layerDto.tiles[i]]
                       : nullptr;
        }

        segment->layers.emplace_back(
            tiles,
            layerDto.dimensions.y,
            layerDto.dimensions.x,
            layerDto.tileSize,
            layerDto.layerName);
    }
}

bool TilemapResource::LoadFromFile(const ResourceSettings& settings)
{
    auto segmentDto = ProcessMap(settings.path);
    DtoToSegment(&mapSegment, segmentDto);
    mapSegment.name = StringId(settings.resourceName);

    return true;
}
