#pragma once
#include "Renderer/Renderer.hpp"
#include <memory>

#include "System/Input.hpp"
#include "UiElement.hpp"

class Input;
class Renderer;

struct LabelStyle
{
    LabelStyle() = default;

    LabelStyle(const FontSettings& font_)
        : font(font_)
    {

    }

    FontSettings font;
    std::string text;
};

class Label : public UiElement
{
public:
    Label* SetStyle(std::shared_ptr<LabelStyle> labelStyle)
    {
        _labelStyle = labelStyle;
        _text = labelStyle->text;
        RequireReformat();
        return this;
    }

    Label* SetText(const std::string& text)
    {
        _text = text;
        RequireReformat();
        return this;
    }

private:
    void DoRendering(Renderer* renderer) override
    {
        auto position = GetAbsolutePosition();
        renderer->RenderString(_labelStyle->font, _text.c_str(), position.XY(), position.z);
    }

    void UpdateSize() override
    {
        _size = _labelStyle->font.spriteFont->GetFont()
            ->MeasureStringWithNewlines(_text.c_str(), 1)
            .AsVectorOfType<float>();
    }

private:
    std::shared_ptr<LabelStyle> _labelStyle;
    std::string _text;
};

enum class ButtonType
{
    Text
};

struct ButtonStyles
{
    std::shared_ptr<BackgroundStyle> releasedStyle;
    std::shared_ptr<BackgroundStyle> pressedStyle;
    std::shared_ptr<BackgroundStyle> hoverStyle;
};

struct LabelButtonStyle
{
    std::shared_ptr<LabelStyle> labelStyle;
    std::shared_ptr<ButtonStyles> buttonStyle;
    float padding = 0;
};

class ButtonStylesBuilder
{
public:
    ButtonStylesBuilder& ReleasedStyle(std::shared_ptr<BackgroundStyle> releasedStyle)
    {
        _style->releasedStyle = releasedStyle;
        return *this;
    }

    ButtonStylesBuilder& PressedStyle(std::shared_ptr<BackgroundStyle> pressedStyle)
    {
        _style->pressedStyle = pressedStyle;
        return *this;
    }

    ButtonStylesBuilder& HoverStyle(std::shared_ptr<BackgroundStyle> hoverStyle)
    {
        _style->hoverStyle = hoverStyle;
        return *this;
    }

    std::shared_ptr<ButtonStyles> Build()
    {
        return _style;
    }

private:
    std::shared_ptr<ButtonStyles> _style = std::make_shared<ButtonStyles>();
};

struct PaddingStyle
{
    float top = 0, bottom = 0, left = 0, right = 0;
};


class Button : public UiElement
{
public:
    Button();

    virtual ~Button() = default;

    Button* SetStyle(std::shared_ptr<ButtonStyles> buttonStyles);
    void SetInternalPadding(PaddingStyle paddingStyle);
    void UseLabel(std::shared_ptr<LabelStyle> labelSyle);

    EventHandler onClicked;
    EventHandler onHover;
    EventHandler onHoveredAway;

private:
    enum class ButtonState
    {
        Released,
        Pressed,
        Hovering
    };

    void DoUpdate(Input* input, float deltaTime) override;
    void SetState(ButtonState state);

    std::shared_ptr<ButtonStyles> _buttonStyles;
    ButtonState _state = ButtonState::Released;
    PaddingStyle _paddingStyle;
};

class Checkbox : public Button
{
public:
    Checkbox()
    {
        onClicked += [=]
        {
            if (!_isRadioButton)
            {
                SetIsChecked(!_isChecked);
            }
            else if (!_isChecked)
            {
                GetRootElement()->VisitElementAndChildren<Checkbox>([=](Checkbox* checkbox)
                {
                    if (checkbox->_isRadioButton && checkbox->_groupName == this->_groupName)
                    {
                        checkbox->SetIsChecked(false);
                    }
                });

                SetIsChecked(true);
            }
        };
    }

    Checkbox* SetIsChecked(bool isChecked)
    {
        if (isChecked != _isChecked)
        {
            _isChecked = isChecked;

            if (_isChecked) onChecked.Invoke();
            else onUnchecked.Invoke();
        }

        return this;
    }

    void AddToRadioButtonGroup(const std::string& groupName)
    {
        _isRadioButton = true;
        _groupName = groupName;
    }

    std::string GroupName() const { return _groupName; }
    bool IsChecked() const { return _isChecked; }

    EventHandler onChecked;
    EventHandler onUnchecked;

private:
    bool _isChecked = false;
    std::string _groupName;
    bool _isRadioButton = false;
};

enum class StackedLayoutDimension
{
    Horizontal = 0,
    Vertical = 1
};

class PaddingPanel : public UiElement
{
public:
    PaddingPanel()
        : _paddingTop(0), _paddingBottom(0), _paddingLeft(0), _paddingRight(0)
    {
        UiElement::AddExistingChild(_childPanel);
    }

    void SetPadding(PaddingStyle& style)
    {
        SetPadding(style.top, style.bottom, style.left, style.right);
    }

    PaddingPanel* SetPadding(float top = 0, float bottom = 0, float left = 0, float right = 0)
    {
        _paddingTop = top;
        _paddingBottom = bottom;
        _paddingLeft = left;
        _paddingRight = right;

        _childPanel->SetRelativePosition(Vector2(left, top));

        RequireReformat();

        return this;
    }

    void UpdateSize() override
    {
        UiElement::UpdateSize();

        _size += Vector2(_paddingRight, _paddingBottom);
    }

    UiElementPtr AddExistingChild(UiElementPtr element) override
    {
        _childPanel->AddExistingChild(element);
        RequireReformat();
        return element;
    }

private:
    std::shared_ptr<UiElement> _childPanel = std::make_shared<UiElement>();

    float _paddingTop;
    float _paddingBottom;
    float _paddingLeft;
    float _paddingRight;
};

class StackedLayoutPanel : public UiElement
{
public:
    StackedLayoutPanel* SetOrientation(StackedLayoutDimension orientation)
    {
        _layout = orientation;
        RequireReformat();
        return this;
    }

    StackedLayoutPanel* SetSpacing(float spacing)
    {
        _spacing = spacing;
        RequireReformat();
        return this;
    }

private:
    void UpdateSize() override
    {
        int layoutDimension = (int)_layout;
        int otherDimension = !layoutDimension;

        Vector2 newSize;

        for (auto& element : _children)
        {
            auto size = element->GetSize();
            newSize[layoutDimension] += size[layoutDimension];
            newSize[otherDimension] = Max(newSize[otherDimension], size[otherDimension]);
        }

        if (!_children.empty())
        {
            newSize[layoutDimension] += _spacing * (_children.size() - 1);
        }

        _size = newSize;
    }

    void DoFormatting() override
    {
        UiElement::DoFormatting();

        Vector2 position;
        int layoutDimension = (int)_layout;
        int otherDimension = !layoutDimension;

        for (auto& child : _children)
        {
            position[otherDimension] = child->GetRelativePositionInternal().XY()[otherDimension];

            child->SetRelativeOffsetInternal(position);
            position[layoutDimension] += _spacing + child->GetSize()[layoutDimension];
        }
    }

    StackedLayoutDimension _layout = StackedLayoutDimension::Vertical;
    float _spacing = 0;
};

class HiddenPanel : public UiElement
{
public:
    UiElementPtr AddExistingChild(UiElementPtr element) override
    {
        UiElement::AddExistingChild(element);

        if (_isHidden)
        {
            _children.clear();
            _hiddenElements.push_back(element);
        }

        return element;
    }

    HiddenPanel* SetIsHidden(bool isHidden)
    {
        if (isHidden != _isHidden)
        {
            std::swap(_hiddenElements, _children);
            _isHidden = isHidden;
            RequireReformat();
        }

        return this;
    }

private:
    std::vector<UiElementPtr> _hiddenElements;
    bool _isHidden = true;
};

class ProgressBar : public UiElement
{
public:
    void SetProgress(float percentage)
    {
        _percentage = Clamp(percentage, 0.0f, 1.0f);
    }

private:
    void DoRendering(Renderer* renderer) override
    {
        auto rectangle = GetAbsoluteBounds();
        rectangle.SetSize(Vector2(_percentage * rectangle.Width(), rectangle.Height()));

        renderer->RenderRectangle(rectangle, Color::Red(), GetAbsolutePosition().z);
    }

    float _percentage = 0;
};

class Slider : public UiElement
{
public:
    Slider();

    float min = 0;
    float max = 1;
    float value = 0;

    EventHandler onChanged;

private:
    void DoRendering(Renderer* renderer) override;
    void DoUpdate(Input* input, float deltaTime) override;

    bool _isSliding = false;
};

class UiCanvas
{
public:
    UiCanvas();

    std::shared_ptr<UiElement> RootPanel() const
    {
        return _rootPanel;
    }

    void Render(Renderer* renderer);
    void Update(Input* input, float deltaTime);
    void Reset();

    UiElementPtr defaultElement;
    EventHandler onReset;

    std::function<void(Input* input, float deltaTime)> onUpdate;
    std::function<void(Renderer* renderer)> onRender;

    static Resource<SoundEffect> changeSelectedItem;
    static SoundEmitter* GetSoundEmitter();

    static void Initialize(SoundManager* soundManager);

private:
    std::shared_ptr<UiElement> _rootRootPanel;
    std::shared_ptr<UiElement> _rootPanel;

    UiElementPtr _hoveredElement;

    Vector2 _oldScreenSize;
    Vector2 _oldMousePosition;
};

inline UiElement* GetSelectedRadioButton(UiElement* container, const std::string& groupName)
{
    UiElement* selectedElement = nullptr;

    container->VisitElementAndChildren<Checkbox>([&](Checkbox* checkbox)
    {
        if (checkbox->GroupName() == groupName && checkbox->IsChecked())
        {
            selectedElement = checkbox;
        }
    });

    return selectedElement;
}

inline void SetSelectedRadioButton(UiElement* container, const std::string& groupName, const std::string& selectedName)
{
    container->VisitElementAndChildren<Checkbox>([&](Checkbox* checkbox)
    {
        if (checkbox->GroupName() == groupName)
        {
            checkbox->SetIsChecked(checkbox->name == selectedName);
        }
    });
}