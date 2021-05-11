#include "SpriteAtlasResource.hpp"
#include "ResourceSettings.hpp"
#include "System/SpriteAtlasSerialization.hpp"
#include "SpriteResource.hpp"

static SpriteAtlasDto LoadSpriteAtlasDto(const nlohmann::json& spriteAtlasContent)
{
    SpriteAtlasDto spriteAtlasDto;

    if (spriteAtlasContent.contains("atlasType") && spriteAtlasContent.contains("topLeftCornerSize"))
    {
        spriteAtlasDto.atlasType = StringId(spriteAtlasContent["atlasType"].get<std::string>().c_str());
        spriteAtlasDto.topLeftCornerSize = Vector2i(spriteAtlasContent["topLeftCornerSize"][0],
            spriteAtlasContent["topLeftCornerSize"][1]);
    }

    int rows = spriteAtlasContent["rows"];
    int cols = spriteAtlasContent["cols"];
    int cellWidth = spriteAtlasContent["cellWidth"];
    int cellHeight = spriteAtlasContent["cellHeight"];

    Vector2 cellSize(cellWidth, cellHeight);
    spriteAtlasDto.spriteSheet.spriteSheetName;
    spriteAtlasDto.spriteSheet.rows = rows;
    spriteAtlasDto.spriteSheet.cols = cols;
    spriteAtlasDto.spriteSheet.cellSize = cellSize.AsVectorOfType<int>();

    auto animations = spriteAtlasContent["animations"].get<std::vector<nlohmann::json>>();
    for(const auto& animation : animations)
    {
        AtlasAnimation newAnimation;
        newAnimation.name = StringId(animation["name"].get<std::string>().c_str());
        newAnimation.fps = animation["fps"].get<int>();
        newAnimation.frames = std::move(animation["frames"].get<std::vector<int>>());

        spriteAtlasDto.animations.push_back(newAnimation);
    }

    return spriteAtlasDto;
}

StringId AddTileSet(const std::string& fileName, const std::string& resourceName, Vector2 tileSize);

bool SpriteAtlasResource::LoadFromFile(const ResourceSettings& settings)
{
    if (!settings.attributes.has_value())
    {
        Log(LogType::Error, "Missing attributes for sprite atlas %s", settings.resourceName);
        return false;
    }

    auto atlasDto = LoadSpriteAtlasDto(settings.attributes.value());
    AddTileSet(settings.path, settings.path, atlasDto.spriteSheet.cellSize.AsVectorOfType<float>());
    auto atlasSprite = GetResource<SpriteResource>(settings.path);

    _resource.emplace(
        atlasSprite,
        atlasDto.animations,
        atlasDto.spriteSheet.rows,
        atlasDto.spriteSheet.cols,
        atlasDto.topLeftCornerSize.AsVectorOfType<float>(),
        atlasDto.spriteSheet.cellSize.AsVectorOfType<float>());

    return true;
}
