#pragma once

#include <string>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_keycode.h>

#include "Math/Vector2.hpp"

enum class InputDeviceType
{
    None,
    Keyboard,
    Mouse,
    Gamepad,
    GamepadAxis,
};

struct KeyboardState
{
    static const int TOTAL_KEYS = 517;

    KeyboardState();

    bool keyIsDown[TOTAL_KEYS]{};
};

enum class MouseButton
{
    Left = 512,
    Right = 513,
    Middle = 514,
    ScrollUp = 515,
    ScrollDown = 516,
};

struct ButtonMapping
{
    ButtonMapping() = default;

    static ButtonMapping Key(SDL_Scancode key)
    {
        return ButtonMapping(key, InputDeviceType::Keyboard);
    }

    static ButtonMapping GamePadButton(int button)
    {
        return ButtonMapping(button, InputDeviceType::Gamepad);
    }

    static ButtonMapping GamePadAxis(int axisId, float threshold, ComparisonFunction comparison)
    {
        return ButtonMapping(axisId, threshold, comparison, InputDeviceType::GamepadAxis);
    }

    static ButtonMapping Mouse(MouseButton button)
    {
        return ButtonMapping(static_cast<int>(button), InputDeviceType::Mouse);
    }

    int index = -1;
    ComparisonFunction comparison = ComparisonFunction::Equal;
    InputDeviceType deviceType = InputDeviceType::None;

    std::string ButtonName() const;
    std::string DeviceType() const;
    std::string AxisDirectionName() const;

private:
    ButtonMapping(int scancode, InputDeviceType deviceType_)
        : index(scancode),
    deviceType(deviceType_)
    {
        
    }

    ButtonMapping(int axisId, float threshold_, ComparisonFunction comparison_, InputDeviceType deviceType_)
        : index(axisId),
        comparison(comparison_),
        deviceType(deviceType_)
    {

    }
};

class Input;
struct InputState;

struct InputButton
{
    InputButton() = default;

    InputButton(const ButtonMapping& b1);
    InputButton(const ButtonMapping& b1, const ButtonMapping& b2);

    InputButton(SDL_Scancode key);
    InputButton(SDL_Scancode key1, SDL_Scancode key2);

    bool IsPressed() const;
    bool IsDown() const;
    bool IsReleased() const;

    ButtonMapping mappings[2];

private:
    static Input* GetInput();

    static bool ButtonIsDown(const InputState* input, const ButtonMapping& mapping);
    bool ButtonIsDown(const InputState* input) const;
};

using KeyBind = InputButton;

class Mouse
{
public:
    bool LeftDown() const { return _leftDown; }
    bool LeftPressed() const { return _leftDown && !_leftWasDown; }
    bool LeftReleased() const { return !_leftDown && _leftWasDown; }

    bool MiddleDown() const { return _middleDown; }
    bool MiddlePressed() const { return _middleDown && !_middleWasDown; }
    bool MiddleReleased() const { return !_middleDown && _middleWasDown; }

    bool RightDown() const { return _rightDown; }
    bool RightPressed() const { return _rightDown && !_rightWasDown; }
    bool RightReleased() const { return !_rightDown && _rightWasDown; }

    Vector2 MousePosition() const { return (Vector2::Unit() / _scaleFactor) * _mousePosition; }

    void Update();
    void SetMouseScale(const Vector2 scale) { _scaleFactor = scale; }

private:
    bool _leftDown = false;
    bool _leftWasDown = false;

    bool _rightDown = false;
    bool _rightWasDown = false;

    bool _middleDown = false;
    bool _middleWasDown = false;

    Vector2 _mousePosition;
    Vector2 _scaleFactor = Vector2::Unit();
};

struct GamePadAxis
{
    float threshold = 0.5f;
    float value = 0.0f;
};

struct GamePadState
{
    static const int TOTAL_BUTTONS = 17;
    static const int TOTAL_AXIS = 8;

    GamePadState();

    bool buttonPressed[TOTAL_BUTTONS];
    GamePadAxis axis[TOTAL_AXIS];
};

struct InputState
{
    GamePadState gamepadState;
    KeyboardState keyboardState;
};

class Input
{
public:
    Input() = default;
    Input(const Input&) = delete;

    void SendKeyDown(const int key)
    {
        currentInputState.keyboardState.keyIsDown[key] = true;
    }

    void SendKeyUp(const int key)
    {
        currentInputState.keyboardState.keyIsDown[key] = false;
    }

    void SendButtonDown(const int button)
    {
        currentInputState.gamepadState.buttonPressed[button] = true;
    }

    void SendButtonUp(const int button)
    {
        currentInputState.gamepadState.buttonPressed[button] = false;
    }

    void SendAxisValue(const int axis, const float value)
    {
        currentInputState.gamepadState.axis[axis].value = value / 32767;
    }

    void SendMouseWheelY(float y)
    {
        _mouseWheelY = y;
    }

    void Clear()
    {
        currentInputState = InputState();
        lastInputState = InputState();
    }

    void SendTap()
    {
        _wasTapped = true;
    }

    bool WasTapped() const
    {
        return _wasTapped;
    }

    float MouseWheelY() const
    {
        return _mouseWheelY;
    }

    static Input* GetInstance() { return &instance; }

    Mouse* GetMouse() { return &_mouse; }

    // Left is -1
    // Right is -2
    // Up is -3
    // Down is -4
    // None is 0
    int ReadInputKeys();

    void Update();

    InputState currentInputState;
    InputState lastInputState;

    ButtonMapping TryRetrievePressedInput() const;

private:
    static Input instance;

    ButtonMapping RetrieveFirstPressedKey() const;
    ButtonMapping RetrieveFirstPressedGamepadButton() const;
    ButtonMapping RetrieveFirstUsedGamepadAxis() const;

    bool KeyIsPressed(int key);
    bool KeyIsDown(int key);

    bool _wasTapped = false;

    // TODO: mouse to Mouse
    float _mouseWheelY = 0;
    float _lastMouseWheelY = 0;
    Mouse _mouse;

};