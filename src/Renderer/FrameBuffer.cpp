#include "FrameBuffer.hpp"
#include "GL/gl3w.h"
#include "System/BinaryStreamReader.hpp"

FrameBuffer::FrameBuffer(TextureFormat format, TextureDataType dataType, int width, int height)
    : _colorBuffer(format, dataType, width, height)
{
    _colorBuffer.UseLinearFiltering();

    glGenFramebuffers(1, &_frameBuffer);
    Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorBuffer.Id(), 0);
    Unbind();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        FatalError("Failed to initialize frame buffer");
    }
}

void FrameBuffer::AttachDepthAndStencilBuffer()
{
    Bind();
    glGenRenderbuffers(1, &_renderBufferObject);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBufferObject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _colorBuffer.Size().x, _colorBuffer.Size().y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _renderBufferObject);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        FatalError("Failed to attach render buffer");
    }

    Unbind();
}

void FrameBuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
}

void FrameBuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
