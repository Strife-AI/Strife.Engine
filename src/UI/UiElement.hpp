#pragma once
#include <functional>
#include <list>

#include "Math/Rectangle.hpp"
#include "Renderer/Renderer.hpp"
#include "System/Input.hpp"

enum class ElementSizing
{
    Auto,
    Fixed
};

enum class BackgroundType
{
    None,
    Colored,
    NineSlice,
    Sprite
};

enum class UiElementShape
{
    Rectangle,
    Circle
};

struct BackgroundStyle
{
    void BackgroundColor(Color color_)
    {
        color = color_;
        type = BackgroundType::Colored;
    }

//    void BackgroundNineSlice(Resource<NineSlice> nineSlice_)
//    {
//        nineSlice = nineSlice_;
//        type = BackgroundType::NineSlice;
//    }
//
//    void BackgroundSprite(Resource<Sprite> sprite_)
//    {
//        sprite = sprite_;
//        type = BackgroundType::Sprite;
//    }

    static std::shared_ptr<BackgroundStyle> FromNineSlice(StringId resourceId);
    static std::shared_ptr<BackgroundStyle> FromSprite(StringId resourceId);
    //static std::shared_ptr<BackgroundStyle> FromSprite(Resource<Sprite> sprite);

    BackgroundType type = BackgroundType::None;
    Color color;
    //Resource<NineSlice> nineSlice;
    //Resource<Sprite> sprite;
};

enum Alignment
{
    None = 0,

    AlignLeft = 1,
    AlignCenterX = 2,
    AlignRight = 4,

    AlignTop = 8,
    AlignCenterY = 16,
    AlignBottom = 32
};

class EventHandler
{
public:
    using IteratorType = std::list<std::function<void()>>::iterator;

    IteratorType operator+=(const std::function<void()>& callback)
    {
        _callbacks.emplace_front(callback);
        return _callbacks.begin();
    }

    void operator-=(IteratorType callback)
    {
        _callbacks.erase(callback);
    }

    void Invoke()
    {
        for (auto& callback : _callbacks)
        {
            callback();
        }
    }

private:
    std::list<std::function<void()>> _callbacks;
};

class UiElement;
using UiElementPtr = std::shared_ptr<UiElement>;

struct UiElementPositioning
{
    Vector3 relativeOffset = Vector3(0, 0, -0.0001f);
    int alignFlags = 0;

    bool HasXAlignment() const
    {
        return (alignFlags & (AlignLeft | AlignCenterX | AlignRight)) != 0;
    }

    bool HasYAlignment() const
    {
        return (alignFlags & (AlignTop | AlignCenterY | AlignBottom)) != 0;
    }
};

class UiElement
{
public:
    virtual UiElementPtr AddExistingChild(UiElementPtr element);

    template<typename T>
    std::shared_ptr<T> AddChild();

    template<typename T>
    std::shared_ptr<T> AddChild(const std::string& name_);

    void RemoveChild(UiElementPtr child);
    void RemoveAllChildren();

    void SetRelativePosition(Vector2 offset);
    Rectangle GetRelativeBounds();
    Rectangle GetAbsoluteBounds();
    Vector3 GetAbsolutePosition();
    Vector2 GetSize();
    void SetFixedSize(Vector2 size);
    void SetRelativeOffsetInternal(Vector2 offset);
    Vector3 GetRelativePositionInternal() const { return _positioning.relativeOffset; }
    void SetIsEnabled(bool isEnabled);

    void Update(Input* input, float deltaTime);
    void Render(Renderer* renderer);

    static UiElementPtr FindHoveredElement(Vector2 mousePosition, UiElementPtr self);

    UiElement* SetBackgroundStyle(std::shared_ptr<BackgroundStyle> style);
    UiElement* SetAlign(int alignFlags);
    void SetShape(UiElementShape shape);
    void SetRelativeDepth(float depth);

    template<typename TElement>
    void VisitElementAndChildren(const std::function<void(TElement*)>& visitCallback);

    template<typename TElement>
    TElement* FindElementByName(const std::string& name);

    UiElement* GetRootElement();

    std::string name;

    UiElementPtr next = nullptr;
    UiElementPtr prev = nullptr;
    bool isHoverable = false;
    bool isFocused = false;

    static void LinkCircular(const std::initializer_list<UiElementPtr>& elements);
    static void LinkCircular(const std::vector<UiElementPtr>& elements);

protected:
    void RequireReformat();
    virtual void DoUpdate(Input* input, float deltaTime);
    virtual void UpdateSize();
    virtual void DoFormatting();
    virtual bool PointInsideElement(Vector2 point);

    UiElement* _parent = nullptr;
    Vector2 _size;
    std::vector<UiElementPtr> _children;
    UiElementShape _shape = UiElementShape::Rectangle;

    bool _isEnabled = true;

private:
    void EnforceUpdatedLayout()
    {
        if (_formattingDirty)
        {
            UpdateSize();
            DoFormatting();

            _formattingDirty = false;
        }
    }

    virtual void DoRendering(Renderer* renderer) { }

    void MarkAbsolutePositionDirty()
    {
        if (_absolutePositionDirty)
        {
            return;
        }
        else
        {
            _absolutePositionDirty = true;

            for (auto& child : _children)
            {
                child->MarkAbsolutePositionDirty();
            }
        }
    }

    void MarkFormattingDirty()
    {
        if (!_formattingDirty)
        {
            _formattingDirty = true;

            if (_parent != nullptr)
            {
                _parent->MarkFormattingDirty();
            }
        }
    }

    ElementSizing _sizing = ElementSizing::Auto;
    UiElementPositioning _positioning;
    std::shared_ptr<BackgroundStyle> _backgroundStyle;

    bool _formattingDirty = true;

    Vector3 _absolutePosition;
    bool _absolutePositionDirty = true;
};

template <typename T>
std::shared_ptr<T> UiElement::AddChild()
{
    auto element = std::make_shared<T>();
    AddExistingChild(element);
    return element;
}

template <typename T>
std::shared_ptr<T> UiElement::AddChild(const std::string& name_)
{
    auto child = AddChild<T>();
    child->name = name_;
    return child;
}

template <typename TElement>
void UiElement::VisitElementAndChildren(const std::function<void(TElement*)>& visitCallback)
{
    auto ptr = dynamic_cast<TElement*>(this);
    if (ptr != nullptr)
    {
        visitCallback(ptr);
    }

    for (auto& child : _children)
    {
        child->VisitElementAndChildren(visitCallback);
    }
}

template <typename TElement>
TElement* UiElement::FindElementByName(const std::string& name)
{
    if (this->name == name)
    {
        auto ptr = dynamic_cast<TElement*>(this);

        if (ptr != nullptr)
        {
            return ptr;
        }
    }

    for (auto& child : _children)
    {
        auto result = child->FindElementByName<TElement>(name);

        if(result != nullptr)
        {
            return result;
        }
    }

    return nullptr;
}
