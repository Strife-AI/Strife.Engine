
#include <slikenet/BitStream.h>

#include "SyncVar.hpp"

#include "Scene/Scene.hpp"

ISyncVar::ISyncVar(SyncVarUpdateFrequency frequency_)
    : frequency(frequency_)
{
    auto entity = Scene::entityUnderConstruction;
    if (entity == nullptr)
    {
        FatalError("Syncvars can only be added to an entity");
    }

    next = entity->syncVarHead;
    entity->syncVarHead = this;
}

void WriteBitsToStream(const unsigned char* data, int size, SLNet::BitStream& stream)
{
    stream.WriteBits(data, size * 8);
}

void ReadBitsFromStream(unsigned char* data, int size, SLNet::BitStream& stream)
{
    stream.ReadBits(data, size * 8);
}

template<>
void ReadSyncVarDelta<Vector2>(const Vector2& before, Vector2& result, SyncVarDeltaMode mode, SLNet::BitStream& stream)
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
void ReadSyncVarDelta<bool>(const bool& before, bool& result, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    result = stream.ReadBit();
}

template<>
void WriteSyncVarDelta<bool>(const bool& before, const bool& after, bool forceFullUpdate, SyncVarDeltaMode mode, SLNet::BitStream& stream)
{
    stream.Write(after);
}

template<>
void WriteSyncVarDelta<Vector2>(const Vector2& before, const Vector2& after, bool forceFullUpdate, SyncVarDeltaMode mode, SLNet::BitStream& stream)
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