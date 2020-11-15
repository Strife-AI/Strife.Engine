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

struct PlayerCommand
{
    unsigned char fixedUpdateCount;
    unsigned char keys;
    unsigned int id;

    uint32 netId;
    bool moveToTarget = false;
    Vector2 target;

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
            stream->Read(b);
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
            stream->ReadBits(&val, sizeof(val) * 8);

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
        v.sizeBytes = NearestPowerOf2(sizeBits, 8);
        v.sizeBits = sizeBits;
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
