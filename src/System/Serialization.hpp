#pragma once

#include <map>
#include <vector>
#include <string>
#include <robin_hood.h>

#include "Math/Rectangle.hpp"
#include "Math/Vector2.hpp"

#include "BinaryStreamReader.hpp"
#include "BinaryStreamWriter.hpp"

template<typename T>
void WriteVector(BinaryStreamWriter& writer, const std::vector<T>& v)
{
    writer.WriteInt(v.size());

    for (auto& item : v)
    {
        Write(writer, item);
    }
}

template<typename T>
void ReadVector(BinaryStreamReader& reader, std::vector<T>& outV)
{
    int size = reader.ReadInt();
    if (size <= 0)
    {
        return;
    }

    outV.resize(size);
    for (auto& item : outV)
    {
        Read(reader, item);
    }
}

template<typename TKey, typename TValue>
void WriteMap(BinaryStreamWriter& writer, const std::map<TKey, TValue>& map)
{
    writer.WriteInt(map.size());

    for (auto& pair
    : map)
    {
        Write(writer, pair.first);
        Write(writer, pair.second);
    }
}

template<typename TKey, typename TValue>
void ReadMap(BinaryStreamReader& reader, std::map<TKey, TValue>& map)
{
    int size = reader.ReadInt();
    for(int i = 0; i < size; ++i)
    {
        TKey key;
        TValue value;

        Read(reader, key);
        Read(reader, value);

        map.insert({ key, value});
    }
}

template<typename TKey, typename TValue>
void WriteMap(BinaryStreamWriter& writer, const robin_hood::unordered_flat_map<TKey, TValue>& map)
{
    writer.WriteInt(map.size());

    for (auto& pair : map)
    {
        Write(writer, pair.first);
        Write(writer, pair.second);
    }
}

template<typename TKey, typename TValue>
void ReadMap(BinaryStreamReader& reader, robin_hood::unordered_flat_map<TKey, TValue>& map)
{
    auto size = reader.ReadInt();
    for (int i = 0; i < size; ++i)
    {
        TKey key;
        TValue value;

        Read(reader, key);
        Read(reader, value);

        map.insert({key, value});
    }
}

void Write(BinaryStreamWriter& writer, int value);

void Read(BinaryStreamReader& reader, int& outValue);

void Write(BinaryStreamWriter& writer, float value);

void Read(BinaryStreamReader& reader, float& outValue);

void Write(BinaryStreamWriter& writer, const std::string& value);

void Read(BinaryStreamReader& reader, std::string& outValue);

void Write(BinaryStreamWriter& writer, const Vector2i& v);

void Read(BinaryStreamReader& reader, Vector2i& outV);

void Write(BinaryStreamWriter& writer, const Rectanglei& rect);

void Read(BinaryStreamReader& reader, Rectanglei& outRect);

