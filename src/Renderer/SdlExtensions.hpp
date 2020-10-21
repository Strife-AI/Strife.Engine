#pragma once

#include <SDL2/SDL.h>

#include "Math/Rectangle.hpp"

template <typename T>
SDL_Rect RectangleToSdlRect(const RectangleTemplate<T>& rect)
{
    SDL_Rect sdlRect;
    sdlRect.x = rect.topLeft.x;
    sdlRect.y = rect.topLeft.y;
    sdlRect.w = rect.Width();
    sdlRect.h = rect.Height();

    return sdlRect;
}