#include <stdexcept>
#include <SDL2/SDL_keyboard.h>
#include <Memory/StringId.hpp>
#include "ConsoleValueParsing.hpp"

#include "Math/Vector2.hpp"
#include "Memory/String.hpp"
#include "System/Input.hpp"

// bool
template<>
ConsoleValueParseResult ParseConsoleValue<bool>(bool& var, const std::string& str)
{
    if(str == "true" || str == "1")
    {
        var = true;

        return ConsoleValueParseResult::Success;
    }
    else if(str == "false" || str == "0")
    {
        var = false;

        return ConsoleValueParseResult::Success;
    }
    else
    {
        return ConsoleValueParseResult::InvalidValue;
    }
}

template<>
std::string SerializeConsoleValue<bool>(const bool& var)
{
    return var
           ? "true"
           : "false";
}

// int

template<>
ConsoleValueParseResult ParseConsoleValue<int>(int& var, const std::string& str)
{
    try
    {
        var = std::stoi(str);

        return ConsoleValueParseResult::Success;
    }
    catch(const std::invalid_argument&)
    {
        return ConsoleValueParseResult::InvalidValue;
    }
    catch(const std::out_of_range&)
    {
        return ConsoleValueParseResult::InvalidRange;
    }
}

template<>
std::string SerializeConsoleValue<int>(const int& var)
{
    return std::to_string(var);
}

// float

template<>
ConsoleValueParseResult ParseConsoleValue<float>(float& var, const std::string& str)
{
    try
    {
        var = std::stof(str);

        return ConsoleValueParseResult::Success;
    }
    catch(const std::invalid_argument&)
    {
        return ConsoleValueParseResult::InvalidValue;
    }
    catch(const std::out_of_range&)
    {
        return ConsoleValueParseResult::InvalidRange;
    }
}

template<>
std::string SerializeConsoleValue<float>(const float& var)
{
    return std::to_string(var);
}

// string

template<>
ConsoleValueParseResult ParseConsoleValue<std::string>(std::string& var, const std::string& str)
{
    var = str;

    return ConsoleValueParseResult::Success;
}

template<>
std::string SerializeConsoleValue<std::string>(const std::string& var)
{
    return var;
}

// Vector2

template<>
ConsoleValueParseResult ParseConsoleValue<Vector2>(Vector2& var, const std::string& str)
{
    try
    {
        auto tokens = SplitString(StripWhitespace(str), ',');

        if(tokens.size() != 2)
        {
            return ConsoleValueParseResult::InvalidValue;
        }

        for(int i = 0; i < 2; ++i)
        {
            var[i] = std::stof(tokens[i]);
        }

        return ConsoleValueParseResult::Success;
    }
    catch (const std::invalid_argument&)
    {
        return ConsoleValueParseResult::InvalidValue;
    }
    catch (const std::out_of_range&)
    {
        return ConsoleValueParseResult::InvalidRange;
    }
}

template<>
std::string SerializeConsoleValue<Vector2>(const Vector2& var)
{
    return std::to_string(var.x) + "," + std::to_string(var.y);
}

// Keybinding
template<>
ConsoleValueParseResult ParseConsoleValue<KeyBind>(KeyBind& var, const std::string& str)
{
    try
    {
        auto tokens = SplitString(StripWhitespace(str), ';');

        if (tokens.size() == 0)
        {
            return ConsoleValueParseResult::InvalidValue;
        }

        InputButton newButton;

        for (int i = 0; i < tokens.size(); ++i)
        {
            auto subTokens = SplitString(StripWhitespace(tokens[i]), ':');

            // Default to Keyboard key if devicetype is missing for migration purposes
            if (subTokens.size() != 1)
            {
#if false
                if (subTokens[0] == "key") inputDevice = InputDeviceType::Keyboard;
                else if (subTokens[0] == "gamepad") inputDevice = InputDeviceType::Gamepad;
                else if (subTokens[0] == "gamepadAxis") inputDevice = InputDeviceType::GamepadAxis;
                else if (subTokens[0] == "mouse") inputDevice = InputDeviceType::Mouse;
                else if (subTokens[0] == "none") inputDevice = InputDeviceType::None;

                newButton.mappings[i].index = (SDL_Scancode)std::stoi(subTokens[1]);
                newButton.mappings[i].deviceType = inputDevice;
#endif
                auto index = (SDL_Scancode)std::stoi(subTokens[1]);

                switch (StringId(subTokens[0]))
                {
                    case "key"_sid:
                        newButton.mappings[i] = ButtonMapping::Key(index);
                        break;

                    case "gamepad"_sid:
                        newButton.mappings[i] = ButtonMapping::GamePadButton(index);
                        break;

                    case "mouse"_sid:
                        newButton.mappings[i] = ButtonMapping::Mouse((MouseButton)index);
                        break;

                    case "gamepadAxis"_sid:
                        newButton.mappings[i] = ButtonMapping::GamePadAxis(index, 0.5f, ComparisonFunction::Greater);
                        break;

                    case "none"_sid:
                    default:
                        newButton.mappings[i] = ButtonMapping();
                }

            }
            else
            {
                newButton.mappings[i] = ButtonMapping::Key((SDL_Scancode)std::stoi(subTokens[0]));
            }
        }

        var = newButton;

        return ConsoleValueParseResult::Success;
    }
    catch (const std::invalid_argument&)
    {
        return ConsoleValueParseResult::InvalidValue;
    }
    catch (const std::out_of_range&)
    {
        return ConsoleValueParseResult::InvalidRange;
    }
}

template<>
std::string SerializeConsoleValue<KeyBind>(const KeyBind& var)
{
    std::string returnValue;

    for (int i = 0; i < 2; ++i)
    {
        returnValue.append(var.mappings[i].DeviceType());
        returnValue.append(":");
        returnValue.append(std::to_string(var.mappings[i].index));

        if (i != 1)
        {
            returnValue.append(";");
        }
    }

    return returnValue;
}