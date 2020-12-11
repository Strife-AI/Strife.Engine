#pragma once

#include "Texture.hpp"

class FrameBuffer
{
public:
    FrameBuffer(TextureFormat format, TextureDataType dataType, int width, int height);

    void AttachDepthAndStencilBuffer();

    void Bind() const;
    void Unbind() const;

    Texture* ColorBuffer() { return &_colorBuffer; }
    int Width() const { return _colorBuffer.Size().x; }
    int Height() const { return _colorBuffer.Size().y; }
    int Id() const { return _frameBuffer; }

private:
    Texture _colorBuffer;
    unsigned int _frameBuffer;
    unsigned int _renderBufferObject;
};
