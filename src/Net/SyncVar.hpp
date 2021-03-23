#pragma once

#include "Math/Vector2.hpp"
#include "Memory/CircularQueue.hpp"

namespace SLNet
{
    class BitStream;
}

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

void WriteBitsToStream(const unsigned char* data, int size, SLNet::BitStream& stream);
void ReadBitsFromStream(unsigned char* data, int size, SLNet::BitStream& stream);

template<typename T>
void WriteSyncVarDelta(const T& before, const T& after, bool forceFullUpdate, SyncVarDeltaMode mode, SLNet::BitStream& stream);

template<typename T>
void ReadSyncVarDelta(const T& before, T& result, SyncVarDeltaMode mode, SLNet::BitStream& stream);

struct ISyncVar
{
    ISyncVar(SyncVarUpdateFrequency frequency_ = SyncVarUpdateFrequency::Frequent);

    ISyncVar(const ISyncVar& rhs) = delete;

    virtual ~ISyncVar() = default;

    virtual bool CurrentValueChangedFromSequence(uint32_t snapshotId) = 0;
    virtual void WriteValueDeltaedFromSnapshot(uint32_t fromSnapshotId, uint32_t toSnapshotId, SLNet::BitStream& stream) = 0;
    virtual void ReadValueDeltaedFromSequence(uint32_t fromSnapshotId, uint32_t toSnapshotId, float time, SLNet::BitStream& stream) = 0;
    virtual void SetCurrentValueToValueAtTime(float time) = 0;
    virtual void AddCurrentValueToSnapshots(uint32_t currentSnapshotId, float currentTime) = 0;

    virtual bool IsBool() = 0;

    ISyncVar* next = nullptr;
    SyncVarUpdateFrequency frequency;
};

template<typename T, typename Enable = void>
struct DirectlySerializableAsBytes
{
    static constexpr bool value = false;
};

template<typename T>
struct DirectlySerializableAsBytes<T, std::enable_if_t<std::is_arithmetic_v<T>>>
{
    static constexpr bool value = true;
};

struct Color;

template<>
struct DirectlySerializableAsBytes<Color> { static constexpr bool value = true; };

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

    bool CurrentValueChangedFromSequence(uint32_t snapshotId) override
    {
        T value;
        if (!TryGetValueAtSnapshot(snapshotId, value))
        {
            // Have to send full value
            return true;
        }

        return value != currentValue;
    }

    void AddCurrentValueToSnapshots(uint32_t currentSnapshotId, float currentTime) override
    {
        AddValue(currentValue, currentTime, currentSnapshotId);
    }

    bool IsBool() override { return std::is_same_v<T, bool>; }

    void WriteValueDeltaedFromSnapshot(uint32_t fromSnapshotId, uint32_t toSnapshotId, SLNet::BitStream& stream) override
    {
        T oldValue;
        bool hasOldValue = TryGetValueAtSnapshot(fromSnapshotId, oldValue);

        T newValue;
        if (!TryGetValueAtSnapshot(toSnapshotId, newValue))
        {
            newValue = currentValue;
        }

        if constexpr (DirectlySerializableAsBytes<T>::value)
        {
            WriteBitsToStream(reinterpret_cast<const unsigned char*>(&newValue), sizeof(T), stream);
        }
        else
        {
            WriteSyncVarDelta(oldValue, newValue, !hasOldValue, deltaMode, stream);
        }
    }

    void ReadValueDeltaedFromSequence(uint32_t fromSnapshotId, uint32_t toSnapshotId, float time, SLNet::BitStream& stream) override
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

        if constexpr (DirectlySerializableAsBytes<T>::value)
        {
            ReadBitsFromStream(reinterpret_cast<unsigned char*>(&newValue), sizeof(T), stream);
        }
        else
        {
            ReadSyncVarDelta(oldValue, newValue, deltaMode, stream);
        }

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

        Snapshot(const T& value_, float time_, uint32_t snapshotId_)
            : value(value_),
            time(time_),
            snapshotId(snapshotId_)
        {

        }

        T value;
        float time;
        uint32_t snapshotId;
    };

    void operator=(const T& value)
    {
        currentValue = value;
    }

    void SetValue(const T& value)
    {
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

    void AddValue(const T& value, float time, uint32_t snapshotId)
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

    bool TryGetValueAtSnapshot(uint32_t snapshotId, T& outValue)
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