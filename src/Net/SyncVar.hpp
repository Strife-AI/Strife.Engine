#pragma once

#include <box2d/b2_block_allocator.h>
#include <slikenet/BitStream.h>
#include "Math/Vector2.hpp"
#include "Memory/CircularQueue.hpp"

enum class SyncVarUpdateFrequency
{
    Frequent,
    Infrequent
};

enum class SyncVarDeltaMode
{
    Full,
    SmallIntegerOffset
};

enum class SyncVarInterpolation
{
    None,
    Linear
};

template<typename T>
bool TryLerp(const T& start, const T& end, float t, T& outValue)
{
    if constexpr (std::is_arithmetic_v<T>)
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


template<typename T>
void WriteSyncVarDelta(const T& before, const T& after, bool forceFullUpdate, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.WriteBits(reinterpret_cast<const unsigned char*>(&after), sizeof(T) * 8);
}

template<typename T>
void ReadSyncVarDelta(const T& before, T& result, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.ReadBits(reinterpret_cast<unsigned char*>(&result), sizeof(T) * 8);
}

template<>
inline void WriteSyncVarDelta<bool>(const bool& before, const bool& after, bool forceFullUpdate, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.Write(after);
}

template<>
inline void WriteSyncVarDelta<Vector2>(const Vector2& before, const Vector2& after, bool forceFullUpdate, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    if (mode == SyncVarDeltaMode::SmallIntegerOffset)
    {
        int beforeX = round(before.x);
        int beforeY = round(before.y);

        int afterX = round(after.x);
        int afterY = round(after.y);

        int offsetX = afterX - beforeX;
        int offsetY = afterY - beforeY;

        if (!forceFullUpdate
            && offsetX >= -11 && offsetX <= 10
            && offsetY >= -11 && offsetY <= 10)
        {
            unsigned int encodedOffsetX = offsetX + 11;
            unsigned int encodedOffsetY = offsetY + 11;
            unsigned int encodedValue = encodedOffsetX * 22 + encodedOffsetY;

            stream.WriteBits(reinterpret_cast<const unsigned char*>(&encodedValue), 9);
        }
        else
        {
            unsigned int magicOffset = 511;
            stream.WriteBits(reinterpret_cast<const unsigned char*>(&magicOffset), 9);
            stream.WriteCasted<short>(afterX);
            stream.WriteCasted<short>(afterY);
        }
    }
    else
    {
        stream.Write(after.x);
        stream.Write(after.y);
    }
}

template<>
inline void ReadSyncVarDelta<Vector2>(const Vector2& before, Vector2& result, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    if (mode == SyncVarDeltaMode::SmallIntegerOffset)
    {
        unsigned int encodedOffset = 0;
        stream.ReadBits(reinterpret_cast<unsigned char*>(&encodedOffset), 9);

        if (encodedOffset == 511)
        {
            stream.ReadCasted<short>(result.x);
            stream.ReadCasted<short>(result.y);
        }
        else
        {
            result.x = round(before.x) + static_cast<int>(encodedOffset / 22) - 11;
            result.y = round(before.y) + static_cast<int>(encodedOffset % 22) - 11;
        }
    }
    else
    {
        stream.Read(result.x);
        stream.Read(result.y);
    }
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

    virtual bool CurrentValueChangedFromSequence(uint32 snapshotId) = 0;
    virtual void WriteValueDeltaedFromSnapshot(uint32 fromSnapshotId, uint32 toSnapshotId, SLNet::BitStream& stream) = 0;
    virtual void ReadValueDeltaedFromSequence(uint32 fromSnapshotId, uint32 toSnapshotId, float time, SLNet::BitStream& stream) = 0;
    virtual void SetCurrentValueToValueAtTime(float time) = 0;
    virtual void AddCurrentValueToSnapshots(uint32 currentSnapshotId, float currentTime) = 0;

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

    SyncVar(const T& value, SyncVarInterpolation interpolation_, SyncVarUpdateFrequency frequency = SyncVarUpdateFrequency::Frequent, SyncVarDeltaMode deltaMode = SyncVarDeltaMode::Full)
        : ISyncVar(frequency),
        currentValue(value),
        interpolation(interpolation_),
        deltaMode(deltaMode)
    {

    }

    bool CurrentValueChangedFromSequence(uint32 snapshotId) override
    {
        T value;
        if (!TryGetValueAtSnapshot(snapshotId, value))
        {
            // Have to send full value
            return true;
        }

        return value != currentValue;
    }

    void AddCurrentValueToSnapshots(uint32 currentSnapshotId, float currentTime) override
    {
        AddValue(currentValue, currentTime, currentSnapshotId);
    }

    bool IsBool() override { return std::is_same_v<T, bool>; }

    void WriteValueDeltaedFromSnapshot(uint32 fromSnapshotId, uint32 toSnapshotId, SLNet::BitStream& stream) override
    {
        T oldValue;
        bool hasOldValue = TryGetValueAtSnapshot(fromSnapshotId, oldValue);

        T newValue;
        if (!TryGetValueAtSnapshot(toSnapshotId, newValue))
        {
            newValue = currentValue;
        }

        WriteSyncVarDelta(oldValue, newValue, !hasOldValue, deltaMode, stream);
    }

    void ReadValueDeltaedFromSequence(uint32 fromSnapshotId, uint32 toSnapshotId, float time, SLNet::BitStream& stream) override
    {
        T oldValue;
        if (!TryGetValueAtSnapshot(fromSnapshotId, oldValue))
        {
            oldValue = currentValue;
        }

        T newValue;
        if (!TryGetValueAtSnapshot(toSnapshotId, newValue))
        {
            newValue = currentValue;
        }

        ReadSyncVarDelta(oldValue, newValue, deltaMode, stream);
        AddValue(newValue, time, toSnapshotId);
    }

    void SetCurrentValueToValueAtTime(float time) override
    {
        auto newValue = GetValueAtTime(time);
        _wasChanged = currentValue != newValue;
        currentValue = newValue;
    }

    struct Snapshot
    {
        Snapshot()
            : value(),
            time(-1),
            snapshotId(0)
        {

        }

        Snapshot(const T& value_, float time_, uint32 snapshotId_)
            : value(value_),
            time(time_),
            snapshotId(snapshotId_)
        {

        }

        T value;
        float time;
        uint32 snapshotId;
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

    T Value() const
    {
        return currentValue;
    }

    bool Changed() const
    {
        return _wasChanged;
    }

    void AddValue(const T& value, float time, uint32 snapshotId)
    {
        if (!snapshots.IsEmpty())
        {
            auto& lastSnapshot = *(--snapshots.end());
            float lastTime = lastSnapshot.time;

            if(time <= lastTime)
                return;
        }

        if (snapshots.IsFull())
        {
            snapshots.Dequeue();
        }

        snapshots.Enqueue({ value, time, snapshotId });
    }

    bool TryGetValueAtSnapshot(uint32 snapshotId, T& outValue)
    {
        for (auto& snapshot : snapshots)
        {
            if (snapshotId == snapshot.snapshotId)
            {
                outValue = snapshot.value;
                return true;
            }
        }

        return false;
    }

    T GetValueAtTime(float time)
    {
        auto it = snapshots.begin();

        auto& last = *(--snapshots.end());

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

            if (nextSnapshot.time >= time)
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

    CircularQueue<Snapshot, 256> snapshots;
    SyncVarInterpolation interpolation;
    SyncVarDeltaMode deltaMode = SyncVarDeltaMode::Full;

private:
    bool _wasChanged = false;
};