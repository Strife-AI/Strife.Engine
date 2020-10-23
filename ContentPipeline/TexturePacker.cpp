#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <System/Logger.hpp>


#include "Math/Vector2.hpp"
#include "System/ResourceFileWriter.hpp"

Uint32 getpixel(SDL_Surface* surface, int x, int y)
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

void putpixel(SDL_Surface* surface, int x, int y, Uint32 pixel)
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

StringId AddPng(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName);

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

bool doRemove = true;

StringId AddEdgePadding(ResourceFileWriter& writer, SDL_Surface* rawSurface, const std::string& resourceName, GridLayout& layout)
{
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

    std::string file = "temp.png";

    int r = IMG_SavePNG(result, file.c_str());
    auto res = AddPng(writer, file, resourceName);

    SDL_FreeSurface(result);

    if (doRemove)
        remove(file.c_str());

    return res;
}

StringId AddTileSet(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName, Vector2 tileSize)
{
    if (writer.HasResource(resourceName))
    {
        return writer.GetKeyByName(resourceName);
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

    auto res = AddEdgePadding(writer, rawSurface, resourceName, layout);
    SDL_FreeSurface(rawSurface);
    return res;
}

StringId AddNineSliceImage(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName, Vector2 cornerSize)
{
    if (writer.HasResource(resourceName))
    {
        return writer.GetKeyByName(resourceName);
    }

    auto rawSurface = IMG_Load(fileName.c_str());

    GridLayout layout;
    layout.columns.emplace_back(0);
    layout.columns.emplace_back(cornerSize.x);
    layout.columns.emplace_back(rawSurface->w - cornerSize.x);
    layout.columns.emplace_back(rawSurface->w);

    layout.rows.emplace_back(0);
    layout.rows.emplace_back(cornerSize.y);
    layout.rows.emplace_back(rawSurface->h - cornerSize.y);
    layout.rows.emplace_back(rawSurface->h);

    auto result = AddEdgePadding(writer, rawSurface, resourceName, layout);

    Vector2 centerTopLeft = cornerSize;
    Vector2 centerBottomRight = Vector2(rawSurface->w, rawSurface->h) - cornerSize;
    Vector2 centerSize = centerBottomRight - centerTopLeft;

    // Create the center repeated texture
    SDL_Surface* center = SDL_CreateRGBSurfaceWithFormat(
        0,
        centerSize.x,
        centerSize.y,
        rawSurface->format->BitsPerPixel,
        rawSurface->format->format);

    for(int i = 0; i < centerSize.y; ++i)
    {
        for(int j = 0; j < centerSize.x; ++j)
        {
            putpixel(center, j, i, getpixel(rawSurface, j + centerTopLeft.x, i + centerTopLeft.y));
        }
    }

    IMG_SavePNG(center, "center.png");

    AddPng(writer, "center.png", resourceName + "-center");
    remove("center.png");

    SDL_FreeSurface(rawSurface);
    SDL_FreeSurface(center);

    return result;
}

StringId AddTextureAtlasImage(ResourceFileWriter& writer, const std::string& fileName, const std::string& resourceName, Vector2 cellSize)
{
    return AddTileSet(writer, fileName, resourceName, cellSize);
}