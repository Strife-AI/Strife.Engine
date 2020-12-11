#pragma once

#include <functional>
#include <utility>


#include "Color.hpp"
#include "Memory/StringId.hpp"
#include "Math/Vector2.hpp"
#include "Memory/ResourceManager.hpp"
#include "System/SpriteAtlasSerialization.hpp"

class Renderer;
class Sprite;
struct AtlasAnimation;

class SpriteAtlas
{
public:
    SpriteAtlas(Resource<Sprite> atlas,
        const std::vector<AtlasAnimation>& animations,
        int rows,
        int cols,
        Vector2 topLeftCornerSize,
        Vector2 cellSize,
        StringId atlasType = "none"_sid)
        : _atlas(atlas),
          _animations(animations),
          _rows(rows),
          _cols(cols),
          _atlasType(atlasType),
          _topLeftCornerSize(topLeftCornerSize),
          _cellSize(cellSize)
    {

    }

    void GetFrame(int frameIndex, Sprite* outSprite) const;
    const AtlasAnimation* GetAnimation(StringId name) const;

private:
    const Resource<Sprite> _atlas;
    const std::vector<AtlasAnimation> _animations;
    const int _rows;
    const int _cols;
    const StringId _atlasType;
    const Vector2 _topLeftCornerSize;
    Vector2 _cellSize;
};

struct FramesPerSecond
{
    FramesPerSecond()
    {

    }

    explicit constexpr FramesPerSecond(int fps_)
        : fps(fps_)
    {
    }

    int fps;
};

constexpr FramesPerSecond operator ""_fps(const unsigned long long fps)
{
    return FramesPerSecond(static_cast<int>(fps));
}

struct AtlasAnimation
{
    AtlasAnimation()
    {

    }

    AtlasAnimation(const AtlasAnimationDto& animationDto)
        : name(animationDto.animationName.key),
        fps(FramesPerSecond(animationDto.fps)),
        frames(animationDto.frames)
    {

    }

    AtlasAnimation(const StringId& name_, FramesPerSecond fps_, const std::vector<int>& frames_)
        : name(name_),
          fps(fps_),
          frames(frames_)
    {
    }

    const StringId name;
    const FramesPerSecond fps;
    const std::vector<int> frames;
};

enum class AnimationPlayMode
{
    Loop,   // Loop
    Hold,   // Play once, then hold the last frame
    Paused
};

enum class AnimatorType
{
    Sprite,
    NineSlice,
    ThreeSlice
};

struct AnimationFrameCallback
{
    AnimationFrameCallback(StringId animationName_, int frameNumber_, const std::function<void()>& callback_)
        : animationName(animationName_),
        frameNumber(frameNumber_),
        callback(callback_)
    {
        
    }

    StringId animationName;
    int frameNumber;
    std::function<void()> callback;
};

class Animator
{
public:
    explicit Animator(Resource<SpriteAtlas> atlas)
        : _atlas(atlas),
          _currentAnimation(nullptr),
          _fullbrightAtlas(nullptr),
          _currentFullbrightAnimation(nullptr)
    {
    }

    Animator()
    {
    }

    Animator(Resource<SpriteAtlas> atlas, Resource<SpriteAtlas> fullbrightAtlas)
        : _atlas(atlas),
        _fullbrightAtlas(fullbrightAtlas),
        _currentAnimation(nullptr),
        _currentFullbrightAnimation(nullptr)
    {

    }

    StringId CurrentAnimation() const
    {
        return _currentAnimation != nullptr
            ? _currentAnimation->name
            : ""_sid;
    }

    AnimationPlayMode GetMode() const { return _mode; }

    bool AnimationComplete() const { return _animationComplete; }

    void RenderAsThreeSlice(Vector2 cornerSize, Vector2 nineSliceSize)
    {
        _cornerSize = cornerSize;
        _type = AnimatorType::ThreeSlice;
        _nineSliceSize = nineSliceSize;
    }

    void PlayAnimation(StringId name, AnimationPlayMode mode);

    void Render(Renderer* renderer, Vector2 position, float depth, float angle = 0);
    void RenderFullbright(Renderer* renderer, Vector2 position, float depth, float angle = 0);

    void SetSpriteAtlas(Resource<SpriteAtlas> atlas);
    void Pause();
    void Resume();
    void GetCurrentFrame(Sprite& outFrame) const;

    void SetFlipHorizontal(bool flipHorizontal)
    {
        _flipHorizontal = flipHorizontal;
    }

    bool IsFlippedHorizontal() const { return _flipHorizontal; }

    void AddFrameCallback(StringId animationName, int frameNumber, const std::function<void()>& callback);
    void RunFrameEnterCallback();

    Color blendColor;

private:
    void NextFrame();

    void RenderInternal(Renderer* renderer, Resource<SpriteAtlas> atlas, const AtlasAnimation* currentAnimation, Vector2 position, float depth, float angle);

    Resource<SpriteAtlas> _atlas = nullptr;
    Resource<SpriteAtlas> _fullbrightAtlas = nullptr;

    const AtlasAnimation* _currentAnimation = nullptr;
    const AtlasAnimation* _currentFullbrightAnimation = nullptr;

    int _currentAnimationIndex;
    AnimationPlayMode _mode;
    AnimationPlayMode _originalMode;
    AnimatorType _type = AnimatorType::Sprite;

    float _relativeTime = 0;
    float _absoluteTime = 0;
    float _frameDuration = 0;

    bool _flipHorizontal = false;
    Vector2 _cornerSize;
    Vector2 _nineSliceSize;
    bool _animationComplete = false;

    std::vector<AnimationFrameCallback> _frameCallbacks;
};
