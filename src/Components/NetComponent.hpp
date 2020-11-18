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

struct PlayerSnapshot
{
    Vector2 position;
    float time;
    int commandId;
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

template <class T>
bool TryLerp(const T& start, const T& end, float t, T& outValue)
{
    if constexpr(std::is_integral_v<T>)
    {
        outValue = Lerp(start, end, t);
        return true;
    }
    else
    {
        return false;
    }
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
    stream.Write(reinterpret_cast<const char*>(&before), sizeof(T));
}

struct ISyncVar
{
    virtual bool CurrentValueChangedFromSequence(uint32 sequenceId) = 0;
    virtual void WriteValueDeltaedFromSequence(uint32 sequenceId, SLNet::BitStream& stream) = 0;
    virtual bool IsBool() = 0;

    static float currentTime;
    static bool isServer;
    static uint32 currentSequenceId;

    ISyncVar* next;
    SyncVarUpdateFrequency frequency;
};

template<typename T>
struct SyncVar : ISyncVar
{
    SyncVar(const T& value)
        : interpolation(SyncVarInterpolation::None)
    {
        AddValue(value, -1, 0);
    }

    SyncVar(const T& value, SyncVarInterpolation interpolation_)
        : interpolation(interpolation_)
    {
        AddValue(value, -1, 0);
    }

    bool CurrentValueChangedFromSequence(uint32 sequenceId) override
    {
        return GetValueAtSequence(sequenceId) == currentValue;
    }

    bool IsBool() override { return std::is_same_v<T, bool>; }

    void WriteValueDeltaedFromSequence(uint32 sequenceId, SLNet::BitStream& stream) override
    {
        
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
        SetValue(value);
    }

    void SetValue(const T& value)
    {
        if (!isServer)
        {
            FatalError("Trying to set syncvar from client");
        }

        auto mostRecent = --snapshots.end();

        if (snapshots.IsEmpty() || (*mostRecent).sequenceId != currentSequenceId)
        {
            ValueInTime snapshot(value, currentTime, currentSequenceId);

            if (snapshots.IsFull())
            {
                snapshots.Dequeue();
            }

            snapshots.Enqueue(snapshot);
        }
        else
        {
            (*mostRecent).value = value;
        }
    }

    void AddValue(const T& value, float time, uint32 sequenceId)
    {
        if(snapshots.IsFull())
        {
            snapshots.Dequeue();
        }

        snapshots.Enqueue({ value, time, sequenceId });
    }

    T GetValueAtSequence(uint32 sequenceId)
    {
        
    }

    T GetValueAtTime(float time)
    {
        auto it = snapshots.begin();

        if(snapshots.IsEmpty())
        {
            FatalError("No values for syncvar");
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

            if(nextSnapshot.time > time)
            {
                if(interpolation == SyncVarInterpolation::Linear)
                {
                    T result;
                    float t = (time - currentSnapshot.time) / (nextSnapshot.time - currentSnapshot.time);
                    if(TryLerp(currentSnapshot.value, nextSnapshot.value, t, result))
                    {
                        return result;
                    }
                }

                return currentSnapshot.value;
            }

            it = next;
        } while(true);
    }

    T currentValue;

    CircularQueue<ValueInTime, 32> snapshots;
    SyncVarInterpolation interpolation;
};

struct NetVar
{
    bool operator==(const NetVar& rhs) const
    {
        if (sizeBytes != rhs.sizeBytes)
        {
            return false;
        }

        return memcmp(data, rhs.data, sizeBytes) == 0;
    }

    uint8* data;
    int sizeBytes;
    int sizeBits;
};

struct NetSerializationState
{
    FixedSizeVector<NetVar, 32> vars;
    FixedSizeVector<uint8, 1024> data;
};

struct NetSerializer
{
    bool Add(bool& b)
    {
        if (isReading)
        {
            auto old = b;
            b = stream->ReadBit();
            return b != old;
        }
        else
        {
            uint8 data = b;
            AddVar(&data, 1);
            return false;
        }
    }

    template<typename T>
    bool Add(T& val)
    {
        if (isReading)
        {
            bool hasNewValue = stream->ReadBit();
            if (!hasNewValue)
                return false;

            T old = val;
            stream->ReadBits((unsigned char*)&val, sizeof(val) * 8);

            return val != old;
        }
        else
        {
            AddVar(&val, sizeof(T) * 8);
            return false;
        }
    }

    void AddVar(void* data, int sizeBits)
    {
        NetVar v;
        v.sizeBytes = NearestPowerOf2(sizeBits, 8) / 8;
        v.sizeBits = sizeBits;
        if (v.sizeBytes == 0) FatalError("Data takes 0 size");
        v.data = Alloc(v.sizeBytes);
        memcpy(v.data, data, v.sizeBytes);
        state->vars.PushBack(v);
    }

    uint8* Alloc(int size)
    {
        uint8* mem = state->data.end();
        state->data.Resize(state->data.Size() + size);
        return mem;
    }

    NetSerializationState* state;
    SLNet::BitStream* stream;
    bool isReading;
};

DEFINE_COMPONENT(NetComponent)
{
    void OnAdded() override;
    void OnRemoved() override;

    Vector2 GetSnapshotPosition(float time);
    void AddSnapshot(const PlayerSnapshot& snapshot);

    int netId;
    int ownerClientId;

    std::vector<PlayerSnapshot> snapshots;

    std::shared_ptr<FlowField> flowField;
    Vector2 acceleration;
};
