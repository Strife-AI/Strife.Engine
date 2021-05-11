#pragma once

#include "Scene/EntityComponent.hpp"
#include "Resource/SpriteAtlasResource.hpp"

enum class AnimationPlayMode
{
    Loop,   // Loop
    Hold,   // Play once, then hold the last frame
    Paused
};

DEFINE_COMPONENT(AnimatorComponent)
{
public:
    explicit AnimatorComponent(SpriteAtlasResource* atlas)
        : _atlas(atlas),
          _currentAnimation(nullptr)
    {

    }

    AnimatorComponent(const char* atlasResourceName);

    StringId CurrentAnimation() const
    {
        return _currentAnimation != nullptr
            ? _currentAnimation->name
            : ""_sid;
    }

    AnimationPlayMode GetMode() const { return _mode; }

    bool AnimationComplete() const { return _animationComplete; }


    void PlayAnimation(StringId name, AnimationPlayMode mode);

    void Render(Renderer* renderer) override;
    void Update(float deltaTime) override;

    void Pause();
    void Resume();
    void GetCurrentFrame(Sprite& outFrame) const;

    void SetFlipHorizontal(bool flipHorizontal)
    {
        _flipHorizontal = flipHorizontal;
    }

    bool IsFlippedHorizontal() const { return _flipHorizontal; }
    void SetSpriteAtlas(SpriteAtlasResource* atlas);

    Color blendColor;
    Vector2 offsetFromCenter;
    float depth;

private:
    void NextFrame();

    SpriteAtlasResource* _atlas = nullptr;

    const AtlasAnimation* _currentAnimation = nullptr;

    int _currentAnimationIndex;
    AnimationPlayMode _mode;
    AnimationPlayMode _originalMode;

    float _relativeTime = 0;
    float _absoluteTime = 0;
    float _frameDuration = 0;

    bool _flipHorizontal = false;
    Vector2 _cornerSize;
    Vector2 _nineSliceSize;
    bool _animationComplete = false;
};