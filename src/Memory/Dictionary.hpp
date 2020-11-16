#pragma once
#include <string_view>
#include <charconv>

#include "Renderer/Color.hpp"
#include "Math/Vector2.hpp"
#include "Memory/StringId.hpp"
#include "System/Logger.hpp"

template<typename T>
bool TryParseDictionaryValue(std::string_view& value, T& outResult)
{
    return false;
}


template<typename TDictionary>
class Dictionary
{
public:
    Dictionary& operator=(const Dictionary& rhs) = default;

    virtual bool HasProperty(const char* key) const
    {
        std::string_view result;
        return TryGetProperty(key, result);
    }

    template<typename T>
    bool TryGetProperty(const char* key, T& outResult) const
    {
        if (static_cast<const TDictionary*>(this)->TryGetCustomProperty(key, outResult))
        {
            return true;
        }
        else
        {
            std::string_view result;
            if (static_cast<const TDictionary*>(this)->TryGetStringViewProperty(key, result))
            {
                return TryParseDictionaryValue(result, outResult);
            }
            else
            {
                return false;
            }
        }
    }


    bool TryGetProperty(const char* key, std::string_view& outValue) const
    {
        return static_cast<const TDictionary*>(this)->TryGetStringViewProperty(key, outValue);
    }


    template<typename T>
    void GetProperty(const char* key, T& outResult) const
    {
        if(!TryGetProperty(key, outResult))
        {
            FatalError("Missing property in dictionary: %s\n", key);
        }
    }

    template<typename T>
    T GetValueOrDefault(const char* key, const T& defaultValue) const
    {
        T value;
        return TryGetProperty(key, value)
            ? value
            : defaultValue;
    }

    std::string_view GetValueOrDefault(const char* key, const std::string_view& defaultValue) const
    {
        std::string_view value;
        return TryGetProperty(key, value)
            ? value
            : defaultValue;
    }
};

template<>
inline bool TryParseDictionaryValue(std::string_view& value, float& outResult)
{
    return sscanf(value.data(), "%f", &outResult) == 1;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, std::string& outResult)
{
    outResult = value;
    return true;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, double& outResult)
{
    return sscanf(value.data(), "%lf", &outResult) == 1;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, int& outResult)
{
    return sscanf(value.data(), "%d", &outResult) == 1;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, unsigned int& outResult)
{
    return sscanf(value.data(), "%u", &outResult) == 1;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, bool& outResult)
{
    if (value == "true" || value == "1")
    {
        outResult = true;
        return true;
    }

    if (value == "false" || value == "0")
    {
        outResult = false;
        return true;
    }

    return false;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, StringId& outResult)
{
    if (value[0] != '#')
    {
        return false;
    }

    return sscanf(value.data() + 1, "%u", &outResult.key) == 1;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, Vector2& outResult)
{
    int fieldsCaptured = sscanf(value.data(), "%f %f", &outResult.x, &outResult.y);
    return fieldsCaptured == 2;
}

template<>
inline bool TryParseDictionaryValue(std::string_view& value, Color& outResult)
{
    float r, g, b, a;;

    int fieldsCaptured = sscanf(value.data(), "%f %f %f %f", &r, &g, &b, &a);

    outResult = Color(r, g, b, a);

    return fieldsCaptured == 4;
}