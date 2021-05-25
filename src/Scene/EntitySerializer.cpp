#include "imgui/imgui.h"

#include <Math/Vector2.hpp>
#include <Color.hpp>

#include "EntitySerializer.hpp"

template<>
std::string SerializeEntityProperty(const Vector2& value)
{
    char buff[256];
    snprintf(buff, sizeof(buff), "%f %f", value.x, value.y);

    return buff;
}

template<>
Vector2 DeserializeEntityProperty(const std::string& value)
{
    Vector2 outResult;
    sscanf(value.data(), "%f %f", &outResult.x, &outResult.y);

    return outResult;
}

template<>
void RenderEntityPropertyUi(const char* name, Vector2& value)
{
    float v[2] = {value.x, value.y};
    ImGui::DragFloat2(name, v);

    value.x = v[0];
    value.y = v[1];
}

template<>
std::string SerializeEntityProperty(const Color& value)
{
    char buff[15];
    snprintf(buff, sizeof(char) * 15, "%d %d %d %d", value.r, value.g, value.b, value.a);

    return buff;
}

template<>
Color DeserializeEntityProperty(const std::string& value)
{
    Color outResult;
    sscanf(value.data(), "%hd %hd %hd %hd", &outResult.r, &outResult.g, &outResult.b, &outResult.a);

    return outResult;
}

template<>
void RenderEntityPropertyUi(const char* name, Color& value)
{
    float v[4] = {(float)value.r, (float)value.g, (float)value.b, (float)value.a};
    ImGui::ColorPicker4(name, v);

    value.r = v[0];
    value.g = v[1];
    value.b = v[2];
    value.a = v[3];
}

template<>
std::string SerializeEntityProperty(const std::string& value)
{
    return value;
}

template<>
std::string DeserializeEntityProperty(const std::string& value)
{
    return value;
}

template<>
void RenderEntityPropertyUi(const char* name, std::string& value)
{
    // TODO: Is this right? I don't feel so good about this
    ImGui::InputText(name, value.data(), value.capacity());
}

// TODO: Let Michael do his Template metaprogramming magic to reduce this if he wants to.
template<>
std::string SerializeEntityProperty(const int& value)
{
    return std::to_string(value);
}

template<>
int DeserializeEntityProperty(const std::string& value)
{
    int outValue;
    sscanf(value.c_str(), "%d", &outValue);
    return outValue;
}

template<>
void RenderEntityPropertyUi(const char* name, int& value)
{
    ImGui::DragInt(name, &value);
}

template<>
std::string SerializeEntityProperty(const float& value)
{
    return std::to_string(value);
}

template<>
float DeserializeEntityProperty(const std::string& value)
{
    float outValue;
    sscanf(value.c_str(), "%f", &outValue);
    return outValue;
}

template<>
void RenderEntityPropertyUi(const char* name, float& value)
{
    ImGui::DragFloat(name, &value);
}

template<>
std::string SerializeEntityProperty(const bool& value)
{
    return std::to_string(value);
}

template<>
bool DeserializeEntityProperty(const std::string& value)
{
    if (value == "true" || value == "1")
    {
        return true;
    }
    else
    {
        return false;
    }
}

template<>
void RenderEntityPropertyUi(const char* name, bool& value)
{
    ImGui::Checkbox(name, &value);
}

