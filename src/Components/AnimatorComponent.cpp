#include "AnimatorComponent.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Sprite.hpp"
#include "Scene/Scene.hpp"

AnimatorComponent::AnimatorComponent(const char* atlasResourceName)
    : _atlas(GetResource<SpriteAtlasResource>(atlasResourceName))
{

}


const AtlasAnimation* SpriteAtlas::GetAnimation(StringId name) const
{
    for (const auto& animation : _animations)
    {
        if (animation.name == name)
        {
            return &animation;
        }
    }

    FatalError("No such animation: %s", name.ToString());
}

void AnimatorComponent::PlayAnimation(StringId name, AnimationPlayMode mode)
{
    if (_atlas == nullptr) return;

    _currentAnimation = _atlas->Get().GetAnimation(name);

    _frameDuration = 1.0f / static_cast<float>(_currentAnimation->fps);
    _relativeTime = 0;
    _currentAnimationIndex = 0;
    _mode = mode;
    _animationComplete = false;
}

void AnimatorComponent::Render(Renderer* renderer)
{
    if (_currentAnimation == nullptr)
    {
        return;
    }

    Sprite currentFrame;
    _atlas->Get().GetFrame(_currentAnimation->frames[_currentAnimationIndex], &currentFrame);

    renderer->RenderSprite(
        &currentFrame,
        owner->ScreenCenter() + offsetFromCenter,
        depth,
        Vector2(1, 1),
        0,
        _flipHorizontal,
        blendColor);
}

void AnimatorComponent::SetSpriteAtlas(SpriteAtlasResource* atlas)
{
    _atlas = atlas;
}

void AnimatorComponent::Pause()
{
    _originalMode = _mode;
    _mode = AnimationPlayMode::Paused;
}

void AnimatorComponent::Resume()
{
    if (_mode == AnimationPlayMode::Paused)
    {
        _mode = _originalMode;
    }
}

void AnimatorComponent::GetCurrentFrame(Sprite& outFrame) const
{
    _atlas->Get().GetFrame(_currentAnimation->frames[_currentAnimationIndex], &outFrame);
}

void AnimatorComponent::NextFrame()
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
}

void AnimatorComponent::Update(float deltaTime)
{
    if (_mode != AnimationPlayMode::Paused)
    {
        // Update current frame
        _relativeTime += GetScene()->relativeTime - _absoluteTime;

        while (_relativeTime >= _frameDuration)
        {
            _relativeTime -= _frameDuration;
            NextFrame();
        }
    }

    _absoluteTime = GetScene()->relativeTime;
}