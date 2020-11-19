#pragma once

#include <vector>
#include <slikenet/BitStream.h>


#include "Math/Vector2.hpp"
#include "Memory/CircularQueue.hpp"
#include "Scene/EntityComponent.hpp"
#include "Scene/IEntityEvent.hpp"

enum class PlayerCommandStatus
{
    NotStarted,
    InProgress,
    Complete
};

Vector2 GetDirectionFromKeyBits(unsigned int keyBits);

DEFINE_EVENT(MoveToEvent)
{
    Vector2 position;
};

DEFINE_EVENT(AttackEvent)
{
    Entity* entity;
};

struct PlayerCommand
{
    unsigned char fixedUpdateCount;
    unsigned char keys;
    unsigned int id;

    uint32 netId;
    bool moveToTarget = false;
    Vector2 target;

    bool attackTarget = false;
    uint32 attackNetId;

    // Client only
    float timeRecorded;

    // Server only
    PlayerCommandStatus status = PlayerCommandStatus::NotStarted;
    int fixedUpdateStartId;
};

DEFINE_EVENT(SpawnedOnClientEvent)
{

};

struct FlowField;

enum class NetVarType
{
    Bytes,
    Vector2
};

enum class SyncVarInterpolation
{
    None,
    Linear
};

template<typename T>
bool TryLerp(const T& start, const T& end, float t, T& outValue)
{
    if constexpr (std::is_integral_v<T>)
    {
        outValue = Lerp(start, end, t);
        return true;
    }
    else
    {
        return false;
    }
}

template<>
inline bool TryLerp(const Vector2& start, const Vector2& end, float t, Vector2& outValue)
{
    outValue = Lerp(start, end, t);
    return true;
}

enum class SyncVarUpdateFrequency
{
    Frequent,
    Infrequent
};

enum class SyncVarDeltaMode
{
    Full,
    Delta
};

template<typename T>
void WriteSyncVarDelta(const T& before, const T& after, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.WriteBits(reinterpret_cast<const unsigned char*>(&after), sizeof(T) * 8);
}

template<typename T>
void ReadSyncVarDelta(const T& before, T& result, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.ReadBits(reinterpret_cast<unsigned char*>(&result), sizeof(T) * 8);
}

template<>
inline void WriteSyncVarDelta<bool>(const bool& before, const bool& after, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.Write(after);
}

template<>
inline void ReadSyncVarDelta<bool>(const bool& before, bool& result, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    result = stream.ReadBit();
}


struct ISyncVar
{
    ISyncVar(SyncVarUpdateFrequency frequency_ = SyncVarUpdateFrequency::Frequent);

    ISyncVar(const ISyncVar& rhs) = delete;

    virtual ~ISyncVar() = default;

    virtual bool CurrentValueChangedFromSequence(uint32 sequenceId) = 0;
    virtual void WriteValueDeltaedFromSequence(uint32 sequenceId, SLNet::BitStream& stream) = 0;
    virtual void ReadValueDeltaedFromSequence(uint32 sequenceId, float time, SLNet::BitStream& stream) = 0;
    virtual void SetCurrentValueToValueAtTime(float time) = 0;

    virtual bool IsBool() = 0;

    static bool isServer;

    ISyncVar* next = nullptr;
    SyncVarUpdateFrequency frequency;
};

template<typename T>
struct SyncVar final : ISyncVar
{
    SyncVar(const T& value)
        : currentValue(value),
        interpolation(SyncVarInterpolation::None)
    {

    }

    SyncVar(const T& value, SyncVarInterpolation interpolation_, SyncVarUpdateFrequency frequency = SyncVarUpdateFrequency::Frequent)
        : ISyncVar(frequency),
        currentValue(value),
        interpolation(interpolation_)
    {

    }

    bool CurrentValueChangedFromSequence(uint32 sequenceId) override
    {
        return true;
        //return GetValueAtSequence(sequenceId) == currentValue;
    }

    bool IsBool() override { return std::is_same_v<T, bool>; }

    void WriteValueDeltaedFromSequence(uint32 sequenceId, SLNet::BitStream& stream) override
    {
        WriteSyncVarDelta(T(), currentValue, deltaMode, stream);
    }

    void ReadValueDeltaedFromSequence(uint32 sequenceId, float time, SLNet::BitStream& stream) override
    {
        T newValue;
        ReadSyncVarDelta(T(), newValue, deltaMode, stream);
        AddValue(newValue, time, sequenceId);
    }

    void SetCurrentValueToValueAtTime(float time) override
    {
        currentValue = GetValueAtTime(time);
    }

    struct ValueInTime
    {
        ValueInTime()
            : value(),
            time(-1),
            sequenceId(0)
        {

        }

        ValueInTime(const T& value_, float time_, uint32 sequenceId_)
            : value(value_),
            time(time_),
            sequenceId(sequenceId_)
        {

        }

        T value;
        float time;
        uint32 sequenceId;
    };

    void operator=(const T& value)
    {
        currentValue = value;
    }

    void SetValue(const T& value)
    {
        if (!isServer)
        {
            FatalError("Trying to set syncvar from client");
        }

        currentValue = value;
    }

    void AddValue(const T& value, float time, uint32 sequenceId)
    {
        if (!snapshots.IsEmpty() && time <= (*(--snapshots.end())).time)
        {
            return;
        }

        if (snapshots.IsFull())
        {
            snapshots.Dequeue();
        }

        snapshots.Enqueue({ value, time, sequenceId });
    }

    T GetValueAtSequence(uint32 sequenceId)
    {
        return T();
        //if()
    }

    T GetValueAtTime(float time)
    {
        auto it = snapshots.begin();

        if (snapshots.IsEmpty())
        {
            return currentValue;
        }

        if (time < snapshots.Peek().time)
        {
            return snapshots.Peek().value;
        }

        do
        {
            auto next = it;
            ++next;

            if (next == snapshots.end())
            {
                return (*it).value;
            }

            auto& currentSnapshot = *it;
            auto& nextSnapshot = *next;

            if (nextSnapshot.time > time)
            {
                if (interpolation == SyncVarInterpolation::Linear)
                {
                    T result;
                    float t = (time - currentSnapshot.time) / (nextSnapshot.time - currentSnapshot.time);
                    if (TryLerp(currentSnapshot.value, nextSnapshot.value, t, result))
                    {
                        return result;
                    }
                }

                return currentSnapshot.value;
            }

            it = next;
        } while (true);
    }

    T currentValue;

    CircularQueue<ValueInTime, 256> snapshots;
    SyncVarInterpolation interpolation;
    SyncVarDeltaMode deltaMode = SyncVarDeltaMode::Full;
};

DEFINE_COMPONENT(NetComponent)
{
    void OnAdded() override;
    void OnRemoved() override;

    int netId;
    int ownerClientId;

    std::shared_ptr<FlowField> flowField;   // TODO: shouldn't be here
    Vector2 acceleration;
};
