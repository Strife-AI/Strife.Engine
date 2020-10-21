#include <cassert>

#include "SpriteAtlas.hpp"
#include "Sprite.hpp"
#include "Engine.hpp"
#include "NineSlice.hpp"
#include "Renderer.hpp"

void SpriteAtlas::GetFrame(const int frameIndex, Sprite* outSprite) const
{
    assert(frameIndex < _rows * _cols);

    const int row = frameIndex / _cols;
    const int col = frameIndex - row * _cols;

    const Vector2 cell(col, row);

    *outSprite = Sprite(
        _atlas->GetTexture(),
        Rectangle(
            cell * (_cellSize + Vector2(8, 8)),
            _cellSize));
}

const AtlasAnimation* SpriteAtlas::GetAnimation(StringId name) const
{
    for (int i = 0; i < _animations.size(); ++i)
    {
        if (_animations[i].name == name)
        {
            return &_animations[i];
        }
    }

    FatalError("No such animation: %s", name.ToString());
}

void Animator::PlayAnimation(StringId name, AnimationPlayMode mode)
{
    if (_atlas.Value() == nullptr) return;

    _currentAnimation = _atlas->GetAnimation(name);

    if (_fullbrightAtlas.Value() != nullptr)
    {
        _currentFullbrightAnimation = _fullbrightAtlas->GetAnimation(name);
    }

    _frameDuration = 1.0f / static_cast<float>(_currentAnimation->fps.fps);
    _relativeTime = 0;
    _currentAnimationIndex = 0;
    _mode = mode;
    _animationComplete = false;

    RunFrameEnterCallback();
}

void Animator::RenderInternal(Renderer* renderer, Resource<SpriteAtlas> atlas, const AtlasAnimation* currentAnimation, Vector2 position, float depth, float angle)
{
    if (_mode != AnimationPlayMode::Paused)
    {
        // Update current frame
        _relativeTime += renderer->CurrentTime() - _absoluteTime;

        while (_relativeTime >= _frameDuration)
        {
            _relativeTime -= _frameDuration;
            NextFrame();
        }
    }

    _absoluteTime = renderer->CurrentTime();

    Sprite currentFrame;
    atlas->GetFrame(currentAnimation->frames[_currentAnimationIndex], &currentFrame);

    if (_type == AnimatorType::Sprite)
    {
        renderer->RenderSprite(&currentFrame, position, depth, Vector2(1, 1), angle, _flipHorizontal, blendColor);
    }
    else if(_type == AnimatorType::ThreeSlice)
    {
        renderer->RenderThreeSlice(&currentFrame, _cornerSize.x, Rectangle(position, _nineSliceSize), depth, angle);
    }
}

void Animator::Render(Renderer* renderer, Vector2 position, float depth, float angle)
{
    if (_currentAnimation == nullptr)
    {
        return;
    }

    RenderInternal(renderer, _atlas, _currentAnimation, position, depth, angle);
}

void Animator::RenderFullbright(Renderer* renderer, Vector2 position, float depth, float angle)
{
    if (_currentFullbrightAnimation == nullptr)
    {
        return;
    }

    RenderInternal(renderer, _fullbrightAtlas, _currentFullbrightAnimation, position, depth, angle);
}

void Animator::SetSpriteAtlas(Resource<SpriteAtlas> atlas)
{
    _atlas = atlas;
}

void Animator::Pause()
{
    _originalMode = _mode;
    _mode = AnimationPlayMode::Paused;
}

void Animator::Resume()
{
    if (_mode == AnimationPlayMode::Paused)
    {
        _mode = _originalMode;
    }
}

void Animator::GetCurrentFrame(Sprite& outFrame) const
{
    _atlas->GetFrame(_currentAnimation->frames[_currentAnimationIndex], &outFrame);
}

void Animator::AddFrameCallback(StringId animationName, int frameNumber, const std::function<void()>& callback)
{
    _frameCallbacks.emplace_back(animationName, frameNumber, callback);
}

void Animator::RunFrameEnterCallback()
{
    for(auto& callback : _frameCallbacks)
    {
        if(callback.animationName == _currentAnimation->name && _currentAnimationIndex == callback.frameNumber)
        {
            callback.callback();
        }
    }
}

void Animator::NextFrame()
{
    if(_animationComplete)
    {
        return;
    }

    ++_currentAnimationIndex;

    if (_currentAnimationIndex >= _currentAnimation->frames.size())
    {
        if (_mode == AnimationPlayMode::Loop)
        {
            _currentAnimationIndex = _currentAnimationIndex % _currentAnimation->frames.size();
        }
        else
        {
            _currentAnimationIndex = _currentAnimation->frames.size() - 1;
            _animationComplete = true;
            return;
        }
    }

    RunFrameEnterCallback();
}