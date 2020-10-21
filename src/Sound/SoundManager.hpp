#pragma once
#include "Math/Vector2.hpp"
#include "Memory/FixedSizeVector.hpp"

class MusicTrack;
class SoundEffect;
struct Entity;

enum class SoundLoopMode
{
    Once,
    Repeated
};

class SoundEmitter;
struct SoundListener;
class SoundManager;

class SoundChannel
{
public:
    void Play(SoundEffect* effect, int priority, float volume);
    void PlayGlobal(SoundEffect* effect, int priority, float volume);

    void Stop() const;
    void Pause() const;
    void Resume() const;
    void UpdatePositionFromEmitter();
    bool IsPlaying() const;

    bool isGlobal = false;
    bool isMuted = false;

private:
    int _alSourceId = -1;
    SoundEmitter* _emitter = nullptr;
    bool _hasRelativePosition = false;

    friend class SoundEmitter;
};

class SoundEmitter
{
public:
    static constexpr int MaxChannels = 4;

    SoundEmitter();
    bool HasAnyActiveSounds() const;

    SoundChannel channels[MaxChannels];
    Vector2 position;
    SoundManager* soundManager;
    Entity* owner = nullptr;
};

struct SoundListener
{
    Vector2 position;
};

class SoundManager
{
public:
    static constexpr  int MaxDistance = 1000;

    SoundManager();
    ~SoundManager();

    void AddSoundEmitter(SoundEmitter* emitter, Entity* owner);
    void RemoveSoundEmitter(SoundEmitter* emitter);
    void UpdateActiveSoundEmitters(float deltaTime);
    void SetListenerPosition(Vector2 position, Vector2 velocity);
    Vector2 GetListenerPosition() const { return _listenerPosition; }
    void AddActiveEmitter(SoundEmitter* emitter);

    void PlayMusic(MusicTrack* music);
    void StopMusic();

    void SetMusicVolume(float volume);
    float GetMusicVolume() const;

    void SetSoundEffectVolume(float volume);
    float GetSoundEffectVolume() const;

private:
    FixedSizeVector<SoundEmitter*, 64> _activeSoundEmitters;
    int _currentMusicTrackHandle;
    Vector2 _listenerPosition;
    Vector2 _listenerVelocity;
};