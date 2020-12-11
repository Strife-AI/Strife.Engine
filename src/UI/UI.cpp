#include <SDL2/SDL_gamecontroller.h>
#include "UI.hpp"


#include "Camera.hpp"
//#include "Entity/RenderLayers.hpp"

static bool EnterPressed(Input* input)
{
    return InputButton(ButtonMapping::Key(SDL_SCANCODE_RETURN),
    ButtonMapping::GamePadButton(SDL_CONTROLLER_BUTTON_A)).IsPressed();
}


Button::Button()
{
    isHoverable = true;
}

Button* Button::SetStyle(std::shared_ptr<ButtonStyles> buttonStyles)
{
    _buttonStyles = buttonStyles;
    return this;
}

void Button::SetInternalPadding(PaddingStyle paddingStyle)
{
    _paddingStyle = paddingStyle;
}

void Button::UseLabel(std::shared_ptr<LabelStyle> labelSyle)
{
    UiElement* parent = this;

    if (_paddingStyle.left != 0 || _paddingStyle.right != 0 || _paddingStyle.top != 0 || _paddingStyle.right != 0)
    {
        auto padPanel = AddChild<PaddingPanel>();
        padPanel->SetPadding(_paddingStyle);

        parent = padPanel.get();
    }

    auto label = parent->AddChild<Label>();
    label->SetStyle(labelSyle);
}

void Button::DoUpdate(Input* input, float deltaTime)
{
    if (!_isEnabled)
    {
        return;
    }

    UiElement::DoUpdate(input, deltaTime);
    auto mouse = input->GetMouse();

    if (isFocused)
    {
        bool insideElement = PointInsideElement(mouse->MousePosition());

        if (mouse->LeftPressed() && insideElement)
        {
            SetState(ButtonState::Pressed);
        }
        else if ((mouse->LeftReleased() && _state == ButtonState::Pressed && insideElement) || EnterPressed(input))
        {
            onClicked.Invoke();
            SetState(ButtonState::Hovering);
        }
        else if ((mouse->LeftReleased() && _state == ButtonState::Pressed && !insideElement))
        {
            SetState(ButtonState::Released);
        }
        else if (_state == ButtonState::Released)
        {
            onHover.Invoke();
            SetState(ButtonState::Hovering);
        }
    }
    else
    {
        onHoveredAway.Invoke();
        SetState(ButtonState::Released);
    }
}

void Button::SetState(ButtonState state)
{
    _state = state;

    if (_buttonStyles != nullptr)
    {
        switch (state)
        {
        case ButtonState::Released:
            SetBackgroundStyle(_buttonStyles->releasedStyle);
            break;

        case ButtonState::Pressed:
            SetBackgroundStyle(_buttonStyles->pressedStyle);
            break;

        case ButtonState::Hovering:
            SetBackgroundStyle(_buttonStyles->hoverStyle);
            break;
        }
    }
}

Slider::Slider()
{
    isHoverable = true;
}

void Slider::DoRendering(Renderer* renderer)
{
    auto bounds = GetAbsoluteBounds();
    float z = GetAbsolutePosition().z;

    Vector2 sliderSize(10, bounds.Height());

    renderer->RenderLine(
        bounds.LeftCenter() + Vector2(sliderSize.x, 0) / 2,
        bounds.RightCenter() - Vector2(sliderSize.x, 0) / 2,
        Color(255, 0, 0),
        z);

    auto sliderCenter = bounds.LeftCenter() +
        Vector2((value - min) / (max - min) * (bounds.Width() - sliderSize.x) + sliderSize.x / 2, 0);

    renderer->RenderRectangle({ sliderCenter - sliderSize / 2, sliderSize },
        Color(128, 128, 128),
        z - 0.00001);
}

void Slider::DoUpdate(Input* input, float deltaTime)
{
    UiElement::DoUpdate(input, deltaTime);

    auto mouse = input->GetMouse();
    auto bounds = GetAbsoluteBounds();

    if (isFocused)
    {
        if (mouse->LeftPressed())
        {
            _isSliding = true;
        }
    }

    if (_isSliding)
    {
        if (!mouse->LeftDown())
        {
            _isSliding = false;
        }
        else
        {
            Vector2 sliderSize(10, bounds.Height());

            float oldValue = value;

            value = Clamp(
                min + (max - min) * (mouse->MousePosition().x - bounds.Left() - sliderSize.x / 2) / (bounds.Size().x - sliderSize.x),
                min,
                max);

            if (value != oldValue)
            {
                onChanged.Invoke();
            }
        }
    }
}

UiCanvas::UiCanvas()
    : _rootRootPanel(std::make_shared<UiElement>()),
      _rootPanel(std::make_shared<UiElement>())
{
    _rootPanel->SetFixedSize(Vector2(1024, 768));
    _rootPanel->SetRelativePosition({ 0, 0 });

    _rootRootPanel->AddExistingChild(_rootPanel);
}

void UiCanvas::Render(Renderer* renderer)
{
    auto panelCenter = renderer->GetCamera()->ZoomedScreenSize() / 2;

    Vector2 panelSize(1024, 768);
    _rootPanel->SetRelativePosition(panelCenter - panelSize / 2);
    _rootPanel->SetRelativeDepth(0); //FIXME MW UiRenderLayer - 0.001);

    _rootRootPanel->Render(renderer);

    if (onRender != nullptr)
    {
        onRender(renderer);
    }
}

static SoundEmitter menuEmitter;

Resource<SoundEffect> UiCanvas::changeSelectedItem;

void UiCanvas::Update(Input* input, float deltaTime)
{
    bool playChangeSound = false;

    auto mousePosition = input->GetMouse()->MousePosition();
    if (mousePosition != _oldMousePosition)
    {
        auto hoveredElement = UiElement::FindHoveredElement(mousePosition, _rootRootPanel);

        if (hoveredElement != nullptr && hoveredElement != _hoveredElement)
        {
            hoveredElement->isFocused = true;

            if (_hoveredElement != nullptr)
            {
                _hoveredElement->isFocused = false;
                playChangeSound = true;
            }

            _hoveredElement = hoveredElement;
        }

        _oldMousePosition = mousePosition;
    }

    if (_hoveredElement != nullptr)
    {
        const KeyBind up (ButtonMapping::Key(SDL_SCANCODE_UP));
        const KeyBind left (ButtonMapping::Key(SDL_SCANCODE_LEFT));
        const KeyBind down (ButtonMapping::Key(SDL_SCANCODE_DOWN));
        const KeyBind right (ButtonMapping::Key(SDL_SCANCODE_RIGHT));

        const KeyBind controllerLeft = {
                ButtonMapping::GamePadAxis(SDL_CONTROLLER_AXIS_LEFTX, 0.5f, ComparisonFunction::Less),
                ButtonMapping::GamePadButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        };

        const KeyBind controllerRight = {
                ButtonMapping::GamePadAxis(SDL_CONTROLLER_AXIS_LEFTX, 0.5f, ComparisonFunction::Greater),
                ButtonMapping::GamePadButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        };

        const KeyBind controllerUp = {
                ButtonMapping::GamePadAxis(SDL_CONTROLLER_AXIS_LEFTY, 0.5f, ComparisonFunction::Less),
                ButtonMapping::GamePadButton(SDL_CONTROLLER_BUTTON_DPAD_UP)
        };

        const KeyBind controllerDown = {
                ButtonMapping::GamePadAxis(SDL_CONTROLLER_AXIS_LEFTY, 0.5f, ComparisonFunction::Greater),
                ButtonMapping::GamePadButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        };

        auto oldHoveredElement = _hoveredElement;

        if ((down.IsPressed() || right.IsPressed() || controllerRight.IsPressed() || controllerDown.IsPressed())
        && _hoveredElement->next != nullptr)
        {
            _hoveredElement = _hoveredElement->next;
        }
        else if ((up.IsPressed() || left.IsPressed() || controllerLeft.IsPressed() || controllerUp.IsPressed())
        && _hoveredElement->prev != nullptr)
        {
            _hoveredElement = _hoveredElement->prev;
        }

        if (_hoveredElement != oldHoveredElement)
        {
            oldHoveredElement->isFocused = false;
            _hoveredElement->isFocused = true;
            playChangeSound = true;
        }
    }

    if (playChangeSound)
    {
        menuEmitter.channels[0].PlayGlobal(changeSelectedItem.Value(), 0, 1);
    }

    _rootRootPanel->Update(input, deltaTime);

    if (onUpdate != nullptr)
    {
        onUpdate(input, deltaTime);
    }
}

void UiCanvas::Reset()
{
    if (_hoveredElement != nullptr)
    {
        _hoveredElement->isFocused = false;
    }

    _hoveredElement = defaultElement;

    if (_hoveredElement != nullptr)
    {
        _hoveredElement->isFocused = true;
    }

    onReset.Invoke();
}

SoundEmitter* UiCanvas::GetSoundEmitter()
{
    return &menuEmitter;
}

void UiCanvas::Initialize(SoundManager* soundManager)
{
    menuEmitter.soundManager = soundManager;
}
