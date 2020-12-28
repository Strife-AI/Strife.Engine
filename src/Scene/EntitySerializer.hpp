#pragma once

#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <Engine.hpp>

enum class EntitySerializerMode
{
    Read,
    Write,
    EditorRead,
    EditorReadWrite,
};

template<typename T>
std::string SerializeEntityProperty(const T& value);

template<typename T>
T DeserializeEntityProperty(const std::string& value);

template<typename T>
void RenderEntityPropertyUi(const char* name, T& value);

struct EntitySerializer
{
    template<typename T>
    EntitySerializer& Add(const char* name, T& value)
    {
        if (mode == EntitySerializerMode::Write)
        {
            properties[name] = SerializeEntityProperty(value);
        }
        else if (mode == EntitySerializerMode::Read)
        {
            auto property = properties.find(name);
            if (property != properties.end())
            {
                value = DeserializeEntityProperty<T>(property->second);
            }
        }
        else if (mode == EntitySerializerMode::EditorReadWrite)
        {
            RenderEntityPropertyUi(name, value);
        }

        return *this;
    }

    EntitySerializerMode mode;
    std::unordered_map<std::string, std::string> properties;
};