#include <string>
#include <map>
#include <SDL2/SDL_image.h>
#include <System/UiSerialization.hpp>
#include <Tools/RawFile.hpp>
#include <unordered_map>
#include <Renderer/ThreeSlice.hpp>

#include "Engine.hpp"
#include "Renderer/SpriteAtlas.hpp"
#include "Renderer/SpriteFont.hpp"
#include "Scene/MapSegment.hpp"
#include "Math/Vector2.hpp"
#include "Math/Rectangle.hpp"
#include "Renderer/NineSlice.hpp"
#include "System/ResourceFileReader.hpp"
#include "System/TileMapSerialization.hpp"
#include "System/SpriteAtlasSerialization.hpp"
#include "Renderer/Texture.hpp"
#include "Renderer/SdlManager.hpp"

#include "ResourceManager.hpp"

#include "OALWrapper/OAL_Loaders.h"
#include "Sound/MusicTrack.hpp"
#include "Sound/SoundEffect.hpp"

std::unordered_map<unsigned int, IResource*> ResourceManager::_resources;
std::unordered_map<unsigned int, std::vector<unsigned int>> ResourceManager::_resourceLists;

Engine* ResourceManager::engine;
int ResourceManager::currentGeneration = 0;

void ResourceManager::Cleanup()
{
    for (auto& resource : _resources)
    {
        if (resource.second->cleanup != nullptr)
        {
            resource.second->cleanup(resource.second);
        }

        delete resource.second;
    }

    _resources.clear();
}

IResource* ResourceManager::GetResourceInternal(StringId id)
{
    const auto it = _resources.find(id.key);

    return it != _resources.end()
        ? it->second
        : nullptr;
}

void ResourceManager::AddResourceInternal(StringId id, IResource* resource)
{
    _resources[id.key] = resource;
    resource->generation = currentGeneration;
}

static Texture* LoadTexture(gsl::span<std::byte> data)
{
    //Load image at specified path
    auto ops = SDL_RWFromMem(&data[0], data.size());
    SDL_Surface* loadedSurface = IMG_LoadPNG_RW(ops);

    delete[] data.data();

    if (loadedSurface == nullptr)
    {
        FatalError("Unable to load image! SDL_image Error: %s\n", IMG_GetError());
    }

    auto texture = new Texture(loadedSurface);

    SDL_FreeSurface(loadedSurface);

    return texture;
}

static void CleanupSprite(IResource* resource)
{
    Sprite* sprite = static_cast<InternalResource<Sprite>*>(resource)->data;
    delete sprite;
}

extern ConsoleVar<bool> g_isServer;

void LoadSprite(StringId id, gsl::span<std::byte> data)
{
    if(ResourceManager::HasResource(id))
    {
        return;
    }

    if(g_isServer.Value())
    {
        auto texture = new Texture();
        auto sprite = new Sprite(texture, Rectangle(Vector2::Zero(), texture->Size()));
        ResourceManager::AddResource(id, sprite, CleanupSprite);
    }
    else
    {
        auto texture = LoadTexture(data);

        Uint32 format;
        int access;
        int w;
        int h;

        auto sprite = new Sprite(texture, Rectangle(Vector2::Zero(), texture->Size()));
        ResourceManager::AddResource(id, sprite, CleanupSprite);
    }
}

static void CleanupSoundFx(IResource* resource)
{
    SoundEffect* sound = static_cast<InternalResource<SoundEffect>*>(resource)->data;
    OAL_Sample_Unload(sound->sample);
    delete sound;
}

void LoadSoundFx(StringId id, gsl::span<std::byte> data)
{
    if (ResourceManager::HasResource(id))
    {
        return;
    }

    auto sample = OAL_Sample_LoadFromBuffer(data.data(), data.size());
    auto effect = new SoundEffect(sample);
    ResourceManager::AddResource(id, effect, CleanupSoundFx);
}

static void CleanupMusic(IResource* resource)
{
    MusicTrack* music = static_cast<InternalResource<MusicTrack>*>(resource)->data;
    OAL_Stream_Unload(music->stream);
    delete music;
}

void LoadMusicTrack(StringId id, gsl::span<std::byte> data)
{
    if (ResourceManager::HasResource(id))
    {
        return;
    }

    auto stream = OAL_Stream_LoadFromBuffer(data.data(), data.size());
    OAL_Stream_SetLoop(stream, true);

    auto music = new MusicTrack(stream);
    ResourceManager::AddResource(id, music, CleanupMusic);
}

static void CleanupRawFile(IResource* resource)
{
    RawFile* textPack = static_cast<InternalResource<RawFile>*>(resource)->data;
    delete textPack;
}

void LoadRawFile(StringId id, gsl::span<std::byte> data)
{
    RawFile* pack = new RawFile;
    pack->data.resize(data.size());

    BinaryStreamReader reader;

    reader.Open(data);
    reader.ReadBlob(&pack->data[0], data.size());
    reader.Close();

    ResourceManager::AddResource(id, pack, CleanupRawFile);
}

static void CleanupSpriteAtlas(IResource* resource)
{
    SpriteAtlas* atlas = static_cast<InternalResource<SpriteAtlas>*>(resource)->data;
    delete atlas;
}

void LoadSpriteAtlas(StringId id, gsl::span<std::byte> data)
{
    if (ResourceManager::HasResource(id))
    {
        return;
    }

    SpriteAtlasDto atlasDto;
    BinaryStreamReader reader;
    reader.Open(data);

    atlasDto.Read(reader);

    auto atlasSprite = ResourceManager::GetResource<Sprite>(atlasDto.spriteSheet.spriteSheetName.key);

    std::vector<AtlasAnimation> animations;
    for(const auto& animationDto : atlasDto.animations)
    {
        animations.emplace_back(animationDto);
    }

    auto atlas = new SpriteAtlas(
        atlasSprite,
        animations,
        atlasDto.spriteSheet.rows,
        atlasDto.spriteSheet.cols,
        atlasDto.topLeftCornerSize.AsVectorOfType<float>(),
        atlasDto.spriteSheet.cellSize.AsVectorOfType<float>(),
        atlasDto.atlasType);

    ResourceManager::AddResource(id, atlas, CleanupSpriteAtlas);
}


void CleanupMapSegment(IResource* resource)
{
    auto segment = static_cast<InternalResource<MapSegment>*>(resource)->data;
    for (auto property : segment->tileProperties)
    {
        delete property;
    }

    for (auto& layer : segment->layers)
    {
        delete[] layer.tileMap.Data();
    }
}

MapSegment* LoadMapSegment(StringId id, gsl::span<std::byte> data)
{
    MapSegmentDto segmentDto;
    BinaryStreamReader reader;
    reader.Open(data);

    segmentDto.Read(reader);

    MapSegment* segment = new MapSegment;
    segment->colliders = std::move(segmentDto.rectangleColliders);
    segment->polygonColliders = std::move(segmentDto.polygonalColliders);

    // Load tiles
    for (auto& tilePropertiesDto : segmentDto.tileProperties)
    {
        auto tileBounds = tilePropertiesDto.bounds.As<float>();

        // Expand by a pixel to prevent seams between tiles
        //tileBounds.bottomRight = tileBounds.bottomRight + Vector2(1, 1);

        Sprite* tileSprite = ResourceManager::GetResource<Sprite>(tilePropertiesDto.spriteResource).Value();
        segment->tileProperties.push_back(new TileProperties(
            Sprite(tileSprite->GetTexture(), tileBounds, tilePropertiesDto.bounds.As<float>()),
            0));
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

    bool foundStart = false;
    bool foundEnd = false;

    // Load entities
    for (auto& entity : segmentDto.entities)
    {
        Vector2 position;
        sscanf(entity.properties["position"].c_str(), "%f %f", &position.x, &position.y);

        auto type = entity.properties["type"];

        if(type == "segment-start")
        {
            if(foundStart)
            {
                FatalError("Map segment has duplicate start markers\n");
            }

            foundStart = true;
            segment->startMarker = (position - Vector2(16, 16) / 2).Floor().AsVectorOfType<float>();
        }
        else if (type == "segment-end")
        {
            if (foundEnd)
            {
                FatalError("Map segment has duplicate end markers\n");
            }

            foundEnd = true;
            segment->endMarker = (position + Vector2(16, -16) / 2).Floor().AsVectorOfType<float>();
        }
        else
        {
            EntityInstance instance;
            instance.totalProperties = entity.properties.size();
            instance.properties = std::make_unique<EntityProperty[]>(instance.totalProperties);

            int index = 0;

            for(auto& p : entity.properties)
            {
                instance.properties[index++] = { p.first.c_str(), p.second.c_str() };
            }

            segment->entities.push_back(std::move(instance));
        }
    }

    // TODO
#if false
    if(!foundStart)
    {
        FatalError("Missing start marker\n");
    }

    if(!foundEnd)
    {
        FatalError("Missing end marker");
    }
#endif

    segment->SetProperties(segmentDto.properties);
    segment->name = id;

    ResourceManager::AddResource(id, segment, CleanupMapSegment);

    return segment;
}

static void CleanupNineSlice(IResource * resource)
{
    SpriteFont* spriteFont = static_cast<InternalResource<SpriteFont>*>(resource)->data;
    delete spriteFont;
}

void LoadNineSlice(StringId id, gsl::span<std::byte> data)
{
    if (ResourceManager::HasResource(id))
    {
        return;
    }

    NineSliceDto nineSliceDto;
    BinaryStreamReader reader;
    reader.Open(data);

    nineSliceDto.Read(reader);

    auto sprite = ResourceManager::GetResource<Sprite>(nineSliceDto.spriteName.key);
    auto centerSprite = ResourceManager::GetResource<Sprite>(nineSliceDto.centerSpriteName.key);


    centerSprite->GetTexture()->EnableRepeat();

    auto slice = new NineSlice(sprite, centerSprite, nineSliceDto.topLeftCornerSize.AsVectorOfType<float>());

    ResourceManager::AddResource(id, slice, CleanupNineSlice);
}

static void CleanupThreeSlice(IResource* resource)
{
    ThreeSlice* threeSlice = static_cast<InternalResource<ThreeSlice>*>(resource)->data;
    delete threeSlice;
}

void LoadThreeSlice(StringId id, gsl::span<std::byte> data)
{
    if (ResourceManager::HasResource(id))
    {
        return;
    }

    ThreeSliceDto threeSliceDto;
    BinaryStreamReader reader;
    reader.Open(data);

    threeSliceDto.Read(reader);

    auto sprite = ResourceManager::GetResource<Sprite>(threeSliceDto.spriteName.key);
    auto slice = new ThreeSlice(sprite, threeSliceDto.topLeftCornerSize.AsVectorOfType<float>());

    ResourceManager::AddResource(id, slice, CleanupThreeSlice);
}

static void CleanupSpriteFont(IResource* resource)
{
    SpriteFont* spriteFont = static_cast<InternalResource<SpriteFont>*>(resource)->data;
    delete spriteFont;
}

void LoadSpriteFont(StringId id, gsl::span<std::byte> data)
{
    if (ResourceManager::HasResource(id))
    {
        return;
    }

    SpriteFontDto spriteFontDto;
    BinaryStreamReader reader;
    reader.Open(data);
    spriteFontDto.Read(reader);

    auto fontSprite = ResourceManager::GetResource<Sprite>(spriteFontDto.spriteName);
    auto font = new SpriteFont(fontSprite, spriteFontDto.rows, spriteFontDto.cols);

    ResourceManager::AddResource(id, font, CleanupSpriteFont);
}

void ResourceManager::LoadResourceFile(BinaryStreamReader& fileReader)
{
    ++currentGeneration;

    ResourceFileReader reader(fileReader);
    auto entries = reader.LoadEntries();

    for (auto entry : entries)
    {
        auto resourceData = reader.LoadResource(entry);

        switch (entry.header.type)
        {
        case "png"_sid:
            LoadSprite(entry.header.key, resourceData);
            break;

        case "wav"_sid:
            LoadSoundFx(entry.header.key, resourceData);
            break;

        case "ogg"_sid:
            LoadMusicTrack(entry.header.key, resourceData);
            break;

        case "map"_sid:
            LoadMapSegment(entry.header.key, resourceData);
            break;

        case "atlas"_sid:
            LoadSpriteAtlas(entry.header.key, resourceData);
            break;

        case "nineslice"_sid:
            LoadNineSlice(entry.header.key, resourceData);
            break;

        case "threeslice"_sid:

            break;

        case "spritefont"_sid:
            LoadSpriteFont(entry.header.key, resourceData);
            break;

        case "rawfile"_sid:
            LoadRawFile(entry.header.key, resourceData);
            break;

        default:
            FatalError("Unknown resource type: %u", entry.header.key);
        }

        _resourceLists[entry.header.type].emplace_back(entry.header.key);
    }
}

bool ResourceManager::TryGetResourcesByType(StringId type, std::vector<StringId>& resources)
{
    if (_resourceLists.find(type.key) == _resourceLists.end())
    {
        return false;
    }

    resources.reserve(_resourceLists[type].size());
    for (auto& resource : _resourceLists[type])
    {
        resources.emplace_back(resource);
    }

    return true;
}
