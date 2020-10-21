#include "SoundManager.hpp"


#include "SoundEffect.hpp"
#include "Math/Vector3.hpp"
#include "OALWrapper/OAL_Funcs.h"
#include "OALWrapper/OAL_Sample.h"
#include <string>


#include "MusicTrack.hpp"
#include "Scene/Entity.hpp"
#include "Tools/ConsoleVar.hpp"

using namespace std;

ConsoleVar<float> g_musicVolume("music-volume", 0.5, true);
ConsoleVar<float> g_effectVolume("effect-volume", 0.5, true);

void SoundChannel::Play(SoundEffect* effect, int priority, float volume)
{
    if(isMuted)
    {
        return;
    }

    if (isGlobal)
    {
        PlayGlobal(effect, priority, volume);
        return;
    }

    Stop();

    if((_emitter->owner->TopLeft() - _emitter->soundManager->GetListenerPosition()).LengthSquared() >
        SoundManager::MaxDistance * SoundManager::MaxDistance)
    {
        return;
    }

    _hasRelativePosition = false;

    // Start paused so we can set the position
    _alSourceId = OAL_Sample_Play(OAL_FREE, effect->sample, volume * g_effectVolume.Value() * 2, true, priority);
    _emitter->soundManager->AddActiveEmitter(_emitter);

    // NOTE: sound attenuation only works if the source is mono!
    OAL_Source_SetMinMaxDistance(_alSourceId, 500, 10000000);
    UpdatePositionFromEmitter();
    OAL_Source_SetPositionRelative(_alSourceId, false);

    OAL_Source_SetPaused(_alSourceId, false);
}

void SoundChannel::PlayGlobal(SoundEffect* effect, int priority, float volume)
{
    if (isMuted)
    {
        return;
    }

    _hasRelativePosition = true;
    Stop();
    _alSourceId = OAL_Sample_Play(OAL_FREE, effect->sample, volume * g_effectVolume.Value() * 2, false, priority);
    OAL_Source_SetPositionRelative(_alSourceId, true);

    Vector3 position(0, 0, 0);
    OAL_Source_SetPosition(_alSourceId, &position.x);
    _emitter->soundManager->AddActiveEmitter(_emitter);
}

void SoundChannel::Stop() const
{
    OAL_Source_Stop(_alSourceId);
}

void SoundChannel::Pause() const
{
    OAL_Source_SetPaused(_alSourceId, true);
}

void SoundChannel::Resume() const
{
    OAL_Source_SetPaused(_alSourceId, false);
}

void SoundChannel::UpdatePositionFromEmitter()
{
    if (!_hasRelativePosition)
    {
        if(_emitter->owner != nullptr)
        {
            _emitter->position = _emitter->owner->Center();
        }

        auto position = Vector3(_emitter->position, 0);
        OAL_Source_SetPosition(_alSourceId, &position.x);
    }
}

bool SoundChannel::IsPlaying() const
{
    return OAL_Source_IsPlaying(_alSourceId);
}

SoundEmitter::SoundEmitter()
{
    for(auto& channel : channels)
    {
        channel._emitter = this;
    }
}

bool SoundEmitter::HasAnyActiveSounds() const
{
    for(auto& channel : channels)
    {
        if (channel.IsPlaying()) return true;
    }

    return false;
}

SoundManager::SoundManager()
{
    cOAL_Init_Params oal_parms;
    oal_parms.mlStreamingBufferSize = 64 * 1024;

    if (!OAL_Init(oal_parms))
    {
        FatalError("Failed to initialize OpenAL");
    }

    OAL_SetDistanceModel(eOAL_DistanceModel_Inverse_Clamped);

    _currentMusicTrackHandle = OAL_FREE;
}

SoundManager::~SoundManager()
{
    StopMusic();
    OAL_Close();
}

void SoundManager::AddSoundEmitter(SoundEmitter* emitter, Entity* owner)
{
    emitter->soundManager = this;
    emitter->owner = owner;
    emitter->position = owner->Center();
}

void SoundManager::RemoveSoundEmitter(SoundEmitter* emitter)
{
    for (auto& channel : emitter->channels) channel.Stop();

    _activeSoundEmitters.RemoveSingle(emitter);
}

void SoundManager::UpdateActiveSoundEmitters(float deltaTime)
{
    int aliveIndex = 0;
    for(int i = 0; i < _activeSoundEmitters.Size(); ++i)
    {
        if (_activeSoundEmitters[i]->owner != nullptr)
        {
            _activeSoundEmitters[i]->position = _activeSoundEmitters[i]->owner->Center();

            for (auto& channel : _activeSoundEmitters[i]->channels)
            {
                channel.UpdatePositionFromEmitter();
            }
        }

        if(_activeSoundEmitters[i]->HasAnyActiveSounds())
        {
            _activeSoundEmitters[aliveIndex++] = _activeSoundEmitters[i];
        }
    }

    _activeSoundEmitters.Resize(aliveIndex);
}

void SoundManager::SetListenerPosition(Vector2 position, Vector2 velocity)
{
    Vector3 p(position, 0);
    Vector3 v(0, 0, 0);
    Vector3 forward(1, 0, 0);
    Vector3 up(0, -1, 0);

    OAL_Listener_SetAttributes(&p.x, &v.x, &forward.x, &up.x);

    _listenerPosition = position;
}

void SoundManager::AddActiveEmitter(SoundEmitter* emitter)
{
    _activeSoundEmitters.PushBackUnique(emitter);
}

extern cOAL_Device* gpDevice;

void SoundManager::PlayMusic(MusicTrack* music)
{
    music->stream->ResetBuffersUsed();

    int newHandle = OAL_Stream_Play(_currentMusicTrackHandle, music->stream, g_musicVolume.Value() * 0.150718 * 2, false);
    _currentMusicTrackHandle = newHandle;
}

void SoundManager::StopMusic()
{
    OAL_Source_Stop(_currentMusicTrackHandle);
}

void SoundManager::SetMusicVolume(float volume)
{
    OAL_Source_SetVolume(_currentMusicTrackHandle, volume * 0.150718 * 2);
    g_musicVolume.SetValue(volume);
}

float SoundManager::GetMusicVolume() const
{
    return g_musicVolume.Value();
}

void SoundManager::SetSoundEffectVolume(float volume)
{
    g_effectVolume.SetValue(volume);
}

float SoundManager::GetSoundEffectVolume() const
{
    return g_effectVolume.Value();
}
