#include <SDL_keyboard.h>
#include <SDL.h>

#include "Input.hpp"


#include "Engine.hpp"
#include "imgui.h"

KeyboardState::KeyboardState()
{
    for (int i = 0; i < TOTAL_KEYS; ++i)
    {
        keyIsDown[i] = false;
    }
}

InputButton::InputButton(const ButtonMapping& b1)
    : mappings{b1, ButtonMapping()}
{

}

InputButton::InputButton(const ButtonMapping& b1, const ButtonMapping& b2)
    : mappings{b1, b2}
{

}

InputButton::InputButton(SDL_Scancode key)
    : mappings{ButtonMapping::Key(key), ButtonMapping()}
{

}

InputButton::InputButton(SDL_Scancode key1, SDL_Scancode key2)
    : mappings{ ButtonMapping::Key(key1), ButtonMapping::Key(key2)}
{

}

bool InputButton::IsPressed() const
{
    auto* input = GetInput();
    return ButtonIsDown(&input->currentInputState) && !ButtonIsDown(&input->lastInputState);
}

bool InputButton::IsDown() const
{
    auto* input = GetInput();
    return ButtonIsDown(&input->currentInputState);
}

bool InputButton::IsReleased() const
{
    auto* input = GetInput();
    return ButtonIsDown(&input->currentInputState) && !ButtonIsDown(&input->lastInputState);
}

bool InputButton::ButtonIsDown(const InputState* input, const ButtonMapping& mapping)
{
    switch (mapping.deviceType)
    {
        case InputDeviceType::Keyboard:
        case InputDeviceType::Mouse:
            return input->keyboardState.keyIsDown[mapping.index];

        case InputDeviceType::Gamepad: 
            return input->gamepadState.buttonPressed[mapping.index];

        case InputDeviceType::GamepadAxis:
        {
            auto threshold = input->gamepadState.axis[mapping.index].threshold;
            threshold *= mapping.comparison == ComparisonFunction::Greater ? 1 : -1;

            return (bool) Step(input->gamepadState.axis[mapping.index].value,
                               threshold,
                               mapping.comparison);
        }
        case InputDeviceType::None: 
            return false;
    }

    FatalError("Unhandled input device type: %d", (int)mapping.deviceType);
}

Input* InputButton::GetInput() { return Engine::GetInstance()->GetInput(); }

bool InputButton::ButtonIsDown(const InputState* input) const
{
    for (const auto& mapping : mappings)
    {
        if (ButtonIsDown(input, mapping))
        {
            return true;
        }
    }

    return false;
}

GamePadState::GamePadState()
{
    for (bool& i : buttonPressed)
    {
        i = false;
    }
}

void Mouse::Update()
{
    _leftWasDown = _leftDown;
    _rightWasDown = _rightDown;
    _middleWasDown = _middleDown;

    int x, y;
    int buttons = SDL_GetMouseState(&x, &y);

    bool imguiItemHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();

    _leftDown = !imguiItemHovered && (buttons & SDL_BUTTON(SDL_BUTTON_LEFT));
    _rightDown = !imguiItemHovered && (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT));
    _middleDown = !imguiItemHovered && (buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE));

    _mousePosition = Vector2(x, y);
}

void Input::Update()
{
    lastInputState = currentInputState;

    currentInputState.keyboardState.keyIsDown[(int)MouseButton::ScrollUp] = _mouseWheelY > 0;
    currentInputState.keyboardState.keyIsDown[(int)MouseButton::ScrollDown] = _mouseWheelY < 0;

    currentInputState.keyboardState.keyIsDown[(int)MouseButton::Left] = _mouse.LeftDown();
    currentInputState.keyboardState.keyIsDown[(int)MouseButton::Right] = _mouse.RightDown();
    currentInputState.keyboardState.keyIsDown[(int)MouseButton::Middle] = _mouse.MiddleDown();

    _wasTapped = false;
    _lastMouseWheelY = _mouseWheelY;
    _mouseWheelY = 0;

    _mouse.Update();
}

bool Input::KeyIsPressed(int key)
{
    return currentInputState.keyboardState.keyIsDown[key] && !lastInputState.keyboardState.keyIsDown[key];
}

bool Input::KeyIsDown(int key)
{
    return currentInputState.keyboardState.keyIsDown[key];
}

int Input::ReadInputKeys()
{
    static const char keys[] =      "asdfghjkl;'1234567890-=qwertyuiop[]\\zxcvbnm,./` ";
    static const char shiftKeys[] = "ASDFGHJKL:\"!@#$%^&*()_+QWERTYUIOP{}|ZXCVBNM<>?~ ";

    auto lshift = SDL_SCANCODE_LSHIFT;
    auto rshift = SDL_SCANCODE_RSHIFT;
    auto tab = SDL_SCANCODE_TAB;
    auto backspace = SDL_SCANCODE_BACKSPACE;
    auto left = SDL_SCANCODE_LEFT;
    auto right = SDL_SCANCODE_RIGHT;
    auto up = SDL_SCANCODE_UP;
    auto down = SDL_SCANCODE_DOWN;
    auto enter = SDL_SCANCODE_RETURN;

    for(int i = 0; i < sizeof(keys); ++i)
    {
        if(KeyIsPressed(SDL_GetScancodeFromKey(keys[i])))
        {
            if(KeyIsDown(lshift) || KeyIsDown(rshift))
            {
                return shiftKeys[i];
            }
            else
            {
                return keys[i];
            }
        }
    }

    if(KeyIsPressed(tab)) { return '\t'; }
    if(KeyIsPressed(backspace)) { return '\b'; }
    if(KeyIsPressed(left)) { return -1; }
    if(KeyIsPressed(right)) { return -2; }
    if(KeyIsPressed(up)) { return -3; }
    if(KeyIsPressed(down)) { return -4; }
    if(KeyIsPressed(enter)) { return '\n'; }

    return 0;
}

ButtonMapping Input::RetrieveFirstPressedKey() const
{
    for (int i = 0; i < KeyboardState::TOTAL_KEYS; ++i)
    {
        if (currentInputState.keyboardState.keyIsDown[i])
        {
            return ButtonMapping::Key(static_cast<SDL_Scancode>(i));
        }
    }

    return ButtonMapping();
}

ButtonMapping Input::RetrieveFirstPressedGamepadButton() const
{
    for (int i = 0; i < GamePadState::TOTAL_BUTTONS; ++i)
    {
        if (currentInputState.gamepadState.buttonPressed[i])
        {
            return ButtonMapping::GamePadButton(i);
        }
    }

    return ButtonMapping();
}

ButtonMapping Input::RetrieveFirstUsedGamepadAxis() const
{
    for (int i = 0; i < GamePadState::TOTAL_AXIS; ++i)
    {
        if (currentInputState.gamepadState.axis[i].value > currentInputState.gamepadState.axis[i].threshold
        || currentInputState.gamepadState.axis[i].value < (-1 * currentInputState.gamepadState.axis[i].threshold))
        {
            auto function = currentInputState.gamepadState.axis[i].value > currentInputState.gamepadState.axis[i].threshold ?
                    ComparisonFunction::Greater : ComparisonFunction::Less;

            return ButtonMapping::GamePadAxis(i,  currentInputState.gamepadState.axis[i].threshold, function);
        }
    }

    return ButtonMapping();
}

ButtonMapping Input::TryRetrievePressedInput() const
{
    auto mapping = RetrieveFirstPressedKey();

    // Wtf is this garbage? ｡･ﾟﾟ･(>д<)･ﾟﾟ･｡
    if (mapping.index == (int)MouseButton::Left
        || mapping.index == (int)MouseButton::Right
        || mapping.index == (int)MouseButton::Middle
        || mapping.index == (int)MouseButton::ScrollUp
        || mapping.index == (int)MouseButton::ScrollDown)
    {
        mapping.deviceType = InputDeviceType::Mouse;
        return mapping;
    }
    else if (mapping.deviceType != InputDeviceType::None)
    {
        return mapping;
    }

    mapping = RetrieveFirstPressedGamepadButton();

    if (mapping.deviceType != InputDeviceType::None)
    {
        return mapping;
    }

    mapping = RetrieveFirstUsedGamepadAxis();

    if (mapping.deviceType != InputDeviceType::None)
    {
        return mapping;
    }
    else
    {
        return ButtonMapping();
    }
}


std::string ButtonMapping::ButtonName() const
{
    if (index == -1) return "NONE";
    else if (index == (int)MouseButton::Left) return  "L MOUSE";
    else if (index == (int)MouseButton::Right) return "R MOUSE";
    else if (index == (int)MouseButton::Middle) return  "M MOUSE";
    else if (index == (int)MouseButton::ScrollUp) return  "SCROLL UP";
    else if (index == (int)MouseButton::ScrollDown) return  "SCROLL DOWN";
    else if (deviceType == InputDeviceType::Gamepad) return SDL_GameControllerGetStringForButton((SDL_GameControllerButton) index);
    else if (deviceType == InputDeviceType::GamepadAxis) return SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)index);
    else return  SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)index));

    return "NONE";
}

std::string ButtonMapping::DeviceType() const
{
    switch (deviceType)
    {
        case InputDeviceType::Keyboard:
            return "key";
        case InputDeviceType::Mouse:
            return "mouse";
        case InputDeviceType::Gamepad:
            return "gamepad";
        case InputDeviceType::GamepadAxis:
            return "gamepadAxis";
        case InputDeviceType::None:
            return "none";
    }

    return "";
}

std::string ButtonMapping::AxisDirectionName() const
{
    std::string returnValue;

    bool vertical = index == SDL_CONTROLLER_AXIS_LEFTY || index == SDL_CONTROLLER_AXIS_RIGHTY;
    bool horizontal = index == SDL_CONTROLLER_AXIS_LEFTX || index == SDL_CONTROLLER_AXIS_RIGHTX;

    if (deviceType == InputDeviceType::GamepadAxis)
    {
        switch (index)
        {
            case SDL_CONTROLLER_AXIS_LEFTX:
            case SDL_CONTROLLER_AXIS_LEFTY:
                returnValue.append("l");
                break;
            case SDL_CONTROLLER_AXIS_RIGHTX:
            case SDL_CONTROLLER_AXIS_RIGHTY:
                returnValue.append("r");
                break;
        }

        if (comparison == ComparisonFunction::Less && vertical)
        {
            returnValue.append(" up");
        }
        else if (comparison == ComparisonFunction::Greater && vertical)
        {
            returnValue.append(" down");
        }
        else if (comparison == ComparisonFunction::Less && horizontal)
        {
            returnValue.append(" left");
        }
        else if (comparison == ComparisonFunction::Greater && horizontal)
        {
            returnValue.append(" right");
        }

        return returnValue;
    }
    else
    {
        return "";
    }
}
