#include <iostream>
#include <SDL_image.h>

#include "nlohmann/json.hpp"
#include "tmxlite/Map.hpp"
#include "tmxlite/TileLayer.hpp"

#include "Math/Vector2.hpp"
#include "Renderer/TilemapRenderer.hpp"
#include "Scene/Entity.hpp"
#include "System/ResourceFileWriter.hpp"
#include "System/ResourceFileReader.hpp"

#include "System/TileMapSerialization.hpp"
#include "System/SpriteAtlasSerialization.hpp"
#include "System/UiSerialization.hpp"

#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <zconf.h>
#endif

using json = nlohmann::json;

const Vector2i TileSize = Vector2i(32, 32);

void CheckForMissingKeys(
        const std::string& resource,
        const std::string name,
        const json& object,
        const std::vector<std::string>& keys)
{
    for (const auto& key : keys)
    {
        if(!object.contains(key))
        {
            FatalError("Failed to build %s resource: %s, missing %s key.",
                    resource.c_str(),
                    name.c_str(),
                    key.c_str());
        }
    }
}

//TODO: Make this thing better... should be able to use absolute paths and relative paths...
std::string buildAssetPath(const std::string& contentFilePath, const std::string& assetPath)
{
    //Quick check to see if the filepath points to an absolute path.
    if(assetPath[0] != '.' && assetPath.find_first_of("..") != std::string::npos)
    {
        return assetPath;
    }

    auto directoryPathPos = contentFilePath.find_last_of("\\/");

    std::string fullAssetPath = contentFilePath.substr(0, directoryPathPos);

    fullAssetPath += assetPath;

    //Flip the slashes if the platform is Windows.
#if defined(__linux__) || defined(__APPLE__)
    char slash = '/';
    char unwantedSlash = '\\';
#elif defined(_WIN64) || defined(_Win32)
    char slash = '\\';
    char unwantedSlash = '/';
#endif
    std::replace(fullAssetPath.begin(), fullAssetPath.end(), unwantedSlash, slash);

    return fullAssetPath;
}

std::string ToColor(int r, int g, int b, int a)
{
    return std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b) + " " + std::to_string(a);
}

std::string WorkingDirectory()
{
#ifdef _WIN32
    char path[MAX_PATH] = "";
    GetCurrentDirectoryA(MAX_PATH, path);
#else
    char path[PATH_MAX] = "";
    getcwd(path, PATH_MAX);
#endif
    return path;
}

StringId AddPng(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName)
{
	return writer.WriteResourceFile(fileName, resourceName, "png");
}

StringId AddWav(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName)
{
    return writer.WriteResourceFile(fileName, resourceName, "wav");
}

StringId AddOgg(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName)
{
    return writer.WriteResourceFile(fileName, resourceName, "ogg");
}

StringId AddRawFile(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName)
{
    return writer.WriteResourceFile(fileName, resourceName, "rawfile");
}

StringId AddTextureAtlasImage(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName, Vector2 cellSize);

void AddAtlas(ResourceFileWriter& resourceWriter, const std::string& path, json& data, const std::string& resourceName)
{
    SpriteAtlasDto spriteAtlasDto;

    json spriteAtlasContent;
    if (!path.empty())
    {
        std::fstream spriteAtlasFile(path);
        spriteAtlasFile >> spriteAtlasContent;
        spriteAtlasFile.close();
    }
    else
    {
        spriteAtlasContent = data;
    }

    const std::vector<std::string> spriteAtlasKeys = {"spritePath", "rows", "cols", "animations"};
    const std::vector<std::string> atlasAnimationKeys = {"name", "fps", "frames"};

    CheckForMissingKeys("Atlas (*.x2atlas)", resourceName, spriteAtlasContent, spriteAtlasKeys);

    if (spriteAtlasContent.contains("atlasType") && spriteAtlasContent.contains("topLeftCornerSize"))
    {
        spriteAtlasDto.atlasType = StringId(spriteAtlasContent["atlasType"].get<std::string>().c_str());
        spriteAtlasDto.topLeftCornerSize = Vector2i(spriteAtlasContent["topLeftCornerSize"][0],
                                                   spriteAtlasContent["topLeftCornerSize"][1]);
    }

    int rows = spriteAtlasContent[spriteAtlasKeys[1]];
    int cols = spriteAtlasContent[spriteAtlasKeys[2]];

    auto spritePath = buildAssetPath(path, spriteAtlasContent[spriteAtlasKeys[0]].get<std::string>());

    auto tex = IMG_Load(spritePath.c_str());
    Vector2 cellSize(tex->w / cols, tex->h / rows);
    AddTextureAtlasImage(resourceWriter, spritePath, spritePath, cellSize);
    SDL_FreeSurface(tex);

    spriteAtlasDto.spriteSheet.spriteSheetName = StringId(spritePath.c_str());
    spriteAtlasDto.spriteSheet.rows = rows;
    spriteAtlasDto.spriteSheet.cols = cols;
    spriteAtlasDto.spriteSheet.cellSize = cellSize.AsVectorOfType<int>();

    auto animations = spriteAtlasContent[spriteAtlasKeys[3]].get<std::vector<json>>();
    for(const auto& animation : animations)
    {
        CheckForMissingKeys("Atlas Animation (*.x2atlas)", resourceName, animation, atlasAnimationKeys);

        AtlasAnimationDto newAnimation;
        newAnimation.animationName = StringId(animation[atlasAnimationKeys[0]].get<std::string>().c_str());
        newAnimation.fps = animation[atlasAnimationKeys[1]].get<int>();
        newAnimation.frames = std::move(animation[atlasAnimationKeys[2]].get<std::vector<int>>());

        spriteAtlasDto.animations.push_back(newAnimation);
    }

    BinaryStreamWriter writer;
    spriteAtlasDto.Write(writer);

    auto& binaryData = writer.GetData();
    resourceWriter.WriteResource(&binaryData[0], binaryData.size(), resourceName, "atlas");
}


StringId AddTileSet(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName, Vector2 tileSize);


void AddSpriteFont(ResourceFileWriter& resourceWriter, const std::string& path, json& data, const std::string& resourceName)
{
    SpriteFontDto spriteFontDto;
    json spriteFontContent;

    if(!path.empty())
    {
        std::fstream spriteFontFile(path);
        spriteFontFile >> spriteFontContent;
        spriteFontFile.close();
    }
    else
    {
        spriteFontContent = data;
    }

    const std::vector<std::string> spriteFontKeys = {"spritePath", "rows", "cols"};
    CheckForMissingKeys("SpriteFont (*.x2sfnt)", resourceName, spriteFontContent, spriteFontKeys);

    auto spritePath = buildAssetPath(path, spriteFontContent[spriteFontKeys[0]].get<std::string>());

    AddTileSet(resourceWriter, spritePath, spritePath, Vector2(16, 16));

    spriteFontDto.rows = spriteFontContent[spriteFontKeys[1]];
    spriteFontDto.cols = spriteFontContent[spriteFontKeys[2]];
    spriteFontDto.spriteName = StringId(spritePath.c_str());

    BinaryStreamWriter writer;
    spriteFontDto.Write(writer);

    auto& binaryData = writer.GetData();
    resourceWriter.WriteResource(&binaryData[0], binaryData.size(), resourceName, "spritefont");
}

StringId AddNineSliceImage(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName, Vector2 cornerSize);

void AddNineSlice(ResourceFileWriter& resourceWriter, const std::string& path, json& data, const std::string& resourceName)
{
    NineSliceDto nineSliceDto;
    json nineSliceFileContent;

    if(!path.empty())
    {
        std::fstream nineSliceFile(path);
        nineSliceFile >> nineSliceFileContent;
        nineSliceFile.close();
    }
    else
    {
        nineSliceFileContent = data;
    }

    const std::vector<std::string> nineSliceKeys = {"spritePath", "topLeftCornerSize", "bottomRightCornerSize"};

    CheckForMissingKeys("Nineslice", resourceName, nineSliceFileContent, nineSliceKeys);

    auto spritePath = buildAssetPath(path, nineSliceFileContent[nineSliceKeys[0]].get<std::string>());

    const auto topLeftCornerSize = nineSliceFileContent[nineSliceKeys[1]].get<std::vector<int>>();
    const auto bottomRightCornerSize = nineSliceFileContent[nineSliceKeys[1]].get<std::vector<int>>();

    nineSliceDto.topLeftCornerSize.x = topLeftCornerSize[0];
    nineSliceDto.topLeftCornerSize.y = topLeftCornerSize[1];

    nineSliceDto.bottomRightCornerSize.x = bottomRightCornerSize[0];
    nineSliceDto.bottomRightCornerSize.y = bottomRightCornerSize[1];

    nineSliceDto.spriteName = StringId(spritePath.c_str());
    nineSliceDto.centerSpriteName = StringId((spritePath + "-center").c_str());

    AddNineSliceImage(resourceWriter, spritePath, spritePath, nineSliceDto.topLeftCornerSize.AsVectorOfType<float>());


    BinaryStreamWriter writer;
    nineSliceDto.Write(writer);

    auto& binaryData = writer.GetData();
    resourceWriter.WriteResource(&binaryData[0], binaryData.size(), resourceName, "nineslice");
}

void AddThreeSlice(ResourceFileWriter& resourceWriter, const std::string& path, json& data, const std::string& resourceName)
{
    ThreeSliceDto threeSliceDto;
    json threeSliceContent;

    if(!path.empty())
    {
        std::fstream threeSliceFile(path);
        threeSliceFile >> threeSliceContent;
        threeSliceFile.close();
    }
    else
    {
        threeSliceContent = data;
    }

    const std::vector<std::string> threeSliceKeys = {"spritePath", "topLeftCornerSize"};

    CheckForMissingKeys("ThreeSlice", resourceName, threeSliceContent, threeSliceKeys);

    auto spritePath = buildAssetPath(path, threeSliceContent[threeSliceKeys[0]].get<std::string>());

    AddPng(resourceWriter, spritePath, spritePath);

    const auto topLeftCornerSize = threeSliceContent[threeSliceKeys[1]].get<std::vector<int>>();

    threeSliceDto.topLeftCornerSize.x = topLeftCornerSize[0];
    threeSliceDto.topLeftCornerSize.y = topLeftCornerSize[1];

    threeSliceDto.spriteName = StringId(spritePath.c_str());

    BinaryStreamWriter writer;
    threeSliceDto.Write(writer);

    auto& binaryData = writer.GetData();
    resourceWriter.WriteResource(&binaryData[0], binaryData.size(), resourceName, "threeslice");
}



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


void LoadTileLayer(
        tmx::Map& map,
        tmx::TileLayer& layer,
        std::map<int, ImportTileProperties*>& tilePropertiesByTileId,
        MapSegment* mapSegment,
        MapSegmentDto* mapSegmentDto) {
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
        layerDto.tileSize = TileSize;
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
                        point.x += j * TileSize.x;
                        point.y += i * TileSize.y;
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

                    Vector2i topLeft = Vector2i(j, i) * TileSize;
                    Vector2i bottomRight = Vector2i(j + width, i + height) * TileSize;

                    mapSegmentDto->rectangleColliders.emplace_back(topLeft, bottomRight - topLeft);
                }
            }
        }

        delete[] alreadyUsedInColliderBools;
    }
}

void LoadObjectGroup(tmx::Map& map, tmx::ObjectGroup& layer, MapSegment* mapSegment, MapSegmentDto* mapSegmentDto)
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

MapSegment* AddMap(ResourceFileWriter& resourceWriter, const std::string& path, const std::string& resourceName, const std::string& lightPath = "")
{
    // TODO: This should get mad if the lightpath value is defined but the file is missing.

    MapSegmentDto mapSegmentDto;

    tmx::Map map;
    if (!map.load(path))
    {
        FatalError("Couldn't load map segment %s", path.c_str());
    }

    std::map<int, ImportTileProperties*> tilePropertiesByTileId;
    std::map<int, int> localTileIdByGlobalId;

    auto mapSegment = new MapSegment;

    int nextTileSerializationId = 0;

    for (auto& tileSet : map.getTilesets())
    {
        auto imagePath = tileSet.getImagePath();
        auto imageResource = AddTileSet(resourceWriter, imagePath, imagePath, Vector2(32, 32));

        Vector2i tileSize = Vector2i(tileSet.getTileSize().x, tileSet.getTileSize().y);

        for (const auto& tile : tileSet.getTiles())
        {
            auto tileIndex = Vector2i(tile.imagePosition.x, tile.imagePosition.y) / Vector2i(32, 32);

            auto position = Vector2i(tile.imagePosition.x, tile.imagePosition.y) + tileIndex * Vector2i(8, 8);
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

            auto tileProperties = new ImportTileProperties(
                imageResource,
                tile.ID,
                Rectanglei(position, tileSize),
                shape,
                nextTileSerializationId++);

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
            LoadTileLayer(map, tileLayer, tilePropertiesByTileId, mapSegment, &mapSegmentDto);
        }
        else if (layer->getType() == tmx::Layer::Type::Object)
        {
            auto objectGroup = layer->getLayerAs<tmx::ObjectGroup>();
            LoadObjectGroup(map, objectGroup, mapSegment, &mapSegmentDto);
        }
    }

    for(const auto& prop : map.getProperties())
    {
        mapSegmentDto.properties[prop.getName()] = GetTmxPropertyValue(prop);
    }

    BinaryStreamWriter segmentWriter;
    mapSegmentDto.Write(segmentWriter);
    auto& segmentData = segmentWriter.GetData();
    resourceWriter.WriteResource(&segmentData[0], segmentData.size(), resourceName, "map");

    return mapSegment;
}

void AddContent(ResourceFileWriter& writer, const json& resources)
{
    const std::vector<std::string> resourceKeys = {"type", "name"};

    //TODO: Provide each add method with a JSON parameter and let it determine whether to load from Content.json or another
    //      file.
    for (const auto& resource : resources)
    {
        CheckForMissingKeys("content resource", "project", resource, resourceKeys);

        const StringId resourceType = StringId(resource[resourceKeys[0]].get<std::string>().c_str());
        auto resourceName = resource[resourceKeys[1]].get<std::string>();

        transform(resourceName.begin(), resourceName.end(), resourceName.begin(), ::tolower);

        std::string filePath = "";
        if(resource.contains("path"))
        {
            filePath = resource["path"];
        }

        json data;
        if (resource.contains("data"))
        {
            data = resource["data"];
        }

        switch(resourceType)
        {
            case "sprite"_sid:
                AddPng(writer, filePath, resourceName);
                break;

            case "sfx"_sid:
                AddWav(writer, filePath, resourceName);
                break;

            case "music"_sid:
                AddOgg(writer, filePath, resourceName);
                break;

            case "tilemap"_sid:
            {
                if (resource.contains("lightPath"))
                {
                    AddMap(writer, filePath, resourceName, resource["lightPath"].get<std::string>());
                }
                else
                {
                    AddMap(writer, filePath, resourceName);
                }
                break;
            }

            case "atlas"_sid:
                AddAtlas(writer, filePath, data, resourceName);
                break;

            case "spritefont"_sid:
                AddSpriteFont( writer, filePath, data, resourceName);
                break;

            case "nineslice"_sid:
                AddNineSlice( writer, filePath, data, resourceName);
                break;

            case "threeslice"_sid:
                AddThreeSlice(writer, filePath, data, resourceName);
                break;

            case "rawfile"_sid:
                AddRawFile(writer, filePath, resourceName);
                break;

            default:
                std::cerr << "Unknown content type: " << resourceType.key << "\n";
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " /path/to/content.json" << std::endl;
        return -1;
    }

    std::string outputDirectory;
    std::string contentFilePath;

    for (int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "-o") == 0)
        {
            outputDirectory = argv[i + 1];
        }

        if(strcmp(argv[i], "-i") == 0)
        {
            contentFilePath = argv[i + 1];
        }
    }

    if(contentFilePath.empty())
    {
        contentFilePath = argv[1];
    }

    if(outputDirectory.empty())
    {
        outputDirectory = WorkingDirectory();
    }

    std::fstream file(contentFilePath.c_str());
    json content;
    file >> content;
    file.close();

    const std::vector<std::string> contentKeys = {"resourceDirectory", "projectName", "resources"};

    CheckForMissingKeys("Resource Pack", "project", content, contentKeys);

    const std::string pathToAssets = buildAssetPath(contentFilePath, '/' + content[contentKeys[0]].get<std::string>()) + '/';
    const std::string outputFile = outputDirectory + '/' + content[contentKeys[1]].get<std::string>() + ".x2rp";

#if defined(__linux__) || defined(__APPLE__)
    chdir(pathToAssets.c_str());
#elif defined(_WIN64) || defined(_Win32)
    SetCurrentDirectory(pathToAssets.c_str());
#endif

    ResourceFileWriter writer;

    writer.Initialize(outputFile.c_str());
    AddContent(writer, content[contentKeys[2]]);
    writer.Finish();

    std::cout << "Resource Pak project: " << content[contentKeys[1]].get<std::string>() << ". Successfully created at " << outputFile << std::endl;

    return 0;
}
