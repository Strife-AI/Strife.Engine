#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <variant>

#include "Memory/Grid.hpp"
#include "Scene/BaseEntity.hpp"

enum class FieldType
{
    Int,
    Float,
    Grid,
    Vector2
};

struct StrifeRuntimeException : std::exception
{
    StrifeRuntimeException(const std::string& message_)
        : message(message_)
    {
        
    }

    const char* what() const override
    {
        return message.c_str();
    }

    std::string message;
};

struct BinaryStream
{
    
};

struct Field
{
    FieldType type;
    const char* name;

    Field& operator=(int val)
    {
        switch (type)
        {
        case FieldType::Float: value.floatValue = val; break;
        case FieldType::Int: value.intValue = val; break;
        default: throw StrifeRuntimeException("");  // TODO
        }

        return *this;
    }

    Field& operator=(float val)
    {
        switch (type)
        {
        case FieldType::Float: value.floatValue = val; break;
        case FieldType::Int: value.intValue = val; break;
        default: throw StrifeRuntimeException("");  // TODO
        }

        return *this;
    }

    operator int() const
    {
        switch (type)
        {
        case FieldType::Float: return value.floatValue;
        case FieldType::Int: return value.intValue;
        default: throw StrifeRuntimeException("");  // TODO
        }
    }

    operator float() const
    {
        switch (type)
        {
        case FieldType::Float: return value.floatValue;
        case FieldType::Int: return value.intValue;
        default: throw StrifeRuntimeException("");  // TODO
        }
    }

    void Serialize(BinaryStream& stream) { }

    template<typename T>
    T Get();

    union
    {
        float floatValue;
        int intValue;
        Vector2 vectorValue;
        struct
        {
            bool isCompressed;
            unsigned int* data;
            int rows;
            int cols;
        } gridValue;
    } value;
};

template <typename T>
T Field::Get()
{
    throw StrifeRuntimeException("");   // TODO
}

template<>
inline int Field::Get<int>()
{
    if(type != FieldType::Int)      throw StrifeRuntimeException("");   // TODO
    return value.intValue;
}

struct InputModel
{
    Field& operator[](const char* name)
    {
        auto it = fields.find(name);
        if(it == fields.end())
        {
            throw StrifeRuntimeException("No such field: " + std::string(name));
        }

        return it->second;
    }

    std::map<std::string, Field> fields;
};

template<typename TEntity>
struct NetworkBuilder
{
    NetworkBuilder& AddFloat(const char* name) { return *this; }

    NetworkBuilder& AddGridSensor(const char* name);

    NetworkBuilder& CollectSample(const std::function<void(TEntity* entity, InputModel& model)>& callback)
    {
        return *this;
    }
};

DEFINE_ENTITY(TestEntity, "test")
{
    Vector2 velocity;
};

void TestTest()
{
    NetworkBuilder<TestEntity> builder;

    builder
        .AddFloat("speed")
        .CollectSample([=](auto player, InputModel& model)
        {
            model["speed"] = 10;
        });
}