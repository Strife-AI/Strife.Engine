#include "UiElement.hpp"


#include "Camera.hpp"
#include "UI.hpp"

void UiElement::RemoveChild(UiElementPtr child)
{
    auto position = std::find(_children.begin(), _children.end(), child);

    if (position != _children.end())
    {
        _children.erase(position);
        RequireReformat();
    }
}

void UiElement::RemoveAllChildren()
{
    if (!_children.empty())
    {
        _children.clear();
        RequireReformat();
    }
}

void UiElement::SetRelativePosition(Vector2 offset)
{
    if (offset != _positioning.relativeOffset.XY())
    {
        _positioning.relativeOffset = _positioning.relativeOffset.SetXY(offset);
        RequireReformat();
    }
}

std::shared_ptr<BackgroundStyle> BackgroundStyle::FromNineSlice(StringId resourceId)
{
    auto style = std::make_shared<BackgroundStyle>();
    style->BackgroundNineSlice(ResourceManager::GetResource<NineSlice>(resourceId));
    return style;
}

std::shared_ptr<BackgroundStyle> BackgroundStyle::FromSprite(StringId resourceId)
{
    auto style = std::make_shared<BackgroundStyle>();
    style->BackgroundSprite(ResourceManager::GetResource<Sprite>(resourceId));
    return style;
}

std::shared_ptr<BackgroundStyle> BackgroundStyle::FromSprite(Resource<Sprite> sprite)
{
    auto style = std::make_shared<BackgroundStyle>();
    style->BackgroundSprite(sprite);
    return style;
}

UiElementPtr UiElement::AddExistingChild(UiElementPtr element)
{
    _children.push_back(element);
    element->_parent = this;
    RequireReformat();

    return element;
}

void UiElement::LinkCircular(const std::initializer_list<UiElementPtr>& elements)
{
    auto data = elements.begin();

    for(int i = 0; i < elements.size(); ++i)
    {
        data[i]->prev = i != 0 ? data[i - 1] : data[elements.size() - 1];
        data[i]->next = data[(i + 1) % elements.size()];
    }
}

// TODO: This method is essentially duplicated. I need a way to convert a const vector to an initializer list
void UiElement::LinkCircular(const std::vector<UiElementPtr>& elements)
{
    auto data = elements.begin();
    for (int i = 0; i < elements.size(); ++i)
    {
        data[i]->prev = i != 0 ? data[i-1] : data[elements.size() -1];
        data[i]->next = data[(i + 1) % elements.size()];
    }
}

void UiElement::RequireReformat()
{
    MarkFormattingDirty();
    MarkAbsolutePositionDirty();
}

void UiElement::DoUpdate(Input* input, float deltaTime)
{

}

Vector2 UiElement::GetSize()
{
    EnforceUpdatedLayout();

    return _size;
}

bool UiElement::PointInsideElement(Vector2 point)
{
    switch (_shape)
    {
    case UiElementShape::Rectangle:
        return GetAbsoluteBounds().ContainsPoint(point);
    case UiElementShape::Circle:
    {
        auto bounds = GetAbsoluteBounds();
        auto size = bounds.Size();

        auto maxAxis = Max(size.x, size.y) / 2;

        return (point - bounds.GetCenter()).LengthSquared() < maxAxis * maxAxis;
    }
    default:
        return false;
    }
}

Rectangle UiElement::GetRelativeBounds()
{
    return Rectangle(
        _positioning.relativeOffset.XY(),
        GetSize());
}

Rectangle UiElement::GetAbsoluteBounds()
{
    return Rectangle(
        GetAbsolutePosition().XY(),
        GetSize());
}

void UiElement::SetIsEnabled(bool isEnabled)
{
    _isEnabled = isEnabled;
    RequireReformat();
}

void UiElement::Update(Input* input, float deltaTime)
{
    DoUpdate(input, deltaTime);

    for (auto& child : _children)
    {
        child->Update(input, deltaTime);
    }
}

void UiElement::Render(Renderer* renderer)
{
    EnforceUpdatedLayout();

    if (_backgroundStyle != nullptr)
    {
        auto bounds = GetAbsoluteBounds();
        switch (_backgroundStyle->type)
        {
        case BackgroundType::Colored:
            renderer->RenderRectangle(
                bounds,
                _backgroundStyle->color,
                _absolutePosition.z);

            break;

        case BackgroundType::NineSlice:
            renderer->RenderNineSlice(
                _backgroundStyle->nineSlice.Value(),
                bounds,
                _absolutePosition.z);

            break;

        case BackgroundType::Sprite:
            renderer->RenderSprite(
                _backgroundStyle->sprite.Value(),
                bounds.TopLeft(),
                _absolutePosition.z,
                bounds.Size() / _backgroundStyle->sprite->Bounds().Size());

            break;

        default:
            break;
        }
    }

    //renderer->RenderRectangleOutline(GetAbsoluteBounds(), Color::Red(), 0);

    DoRendering(renderer);

    for (auto& child : _children)
    {
        child->Render(renderer);
    }
}

UiElementPtr UiElement::FindHoveredElement(Vector2 mousePosition, UiElementPtr self)
{
    for(auto child : self->_children)
    {
        auto result = FindHoveredElement(mousePosition, child);

        if(result != nullptr)
        {
            return result;
        }
    }

    if(self->isHoverable && self->PointInsideElement(mousePosition))
    {
        return self;
    }
    else
    {
        return nullptr;
    }
}

UiElement* UiElement::SetBackgroundStyle(std::shared_ptr<BackgroundStyle> style)
{
    _backgroundStyle = style;
    return this;
}

UiElement* UiElement::SetAlign(int alignFlags)
{
    _positioning.alignFlags = alignFlags;
    RequireReformat();
    return this;
}

void UiElement::SetShape(UiElementShape shape)
{
    _shape = shape;
    RequireReformat();
}

void UiElement::SetRelativeDepth(float depth)
{
    _positioning.relativeOffset.z = depth;
    MarkAbsolutePositionDirty();
}

UiElement* UiElement::GetRootElement()
{
    auto element = this;
    while (element->_parent != nullptr)
    {
        element = element->_parent;
    }

    return element;
}

Vector3 UiElement::GetAbsolutePosition()
{
    if (_absolutePositionDirty)
    {
        if (_parent != nullptr)
        {
            _parent->EnforceUpdatedLayout();
            auto newPosition = (_parent->GetAbsolutePosition() + _positioning.relativeOffset);

            _absolutePosition = newPosition.SetXY(newPosition.XY().Floor().AsVectorOfType<float>());
;        }

        _absolutePositionDirty = false;
    }

    return _absolutePosition;
}

void UiElement::SetFixedSize(Vector2 size)
{
    _size = size;
    _sizing = ElementSizing::Fixed;
    RequireReformat();
}

void UiElement::UpdateSize()
{
    if (_sizing == ElementSizing::Fixed)
    {
        return;
    }

    Rectangle bounds(Vector2::Zero(), Vector2::Zero());

    for (auto& child : _children)
    {
        auto childBounds = Rectangle(
            Vector2::Zero(),
            child->GetSize());

        if (!child->_positioning.HasXAlignment())
        {
            childBounds = childBounds.AddPosition(
                Vector2(child->_positioning.relativeOffset.x, 0));
        }

        if (!child->_positioning.HasYAlignment())
        {
            childBounds = childBounds.AddPosition(
                Vector2(0, child->_positioning.relativeOffset.y));
        }

        bounds = bounds.Union(childBounds);
    }

    _size = bounds.Size();
}

void UiElement::DoFormatting()
{
    for (auto child : _children)
    {
        int flags = child->_positioning.alignFlags;
        auto elementBounds = child->GetRelativeBounds();

        auto position = elementBounds.TopLeft();

        if (flags & AlignLeft)
            position.x = 0;
        else if (flags & AlignRight)
            position.x = _size.x - elementBounds.Width();
        else if (flags & AlignCenterX)
            position.x = _size.x / 2 - elementBounds.Width() / 2;

        if (flags & AlignTop)
            position.y = 0;
        else if (flags & AlignBottom)
            position.y = _size.y - elementBounds.Height();
        else if (flags & AlignCenterY)
            position.y = _size.y / 2 - elementBounds.Height() / 2;

        child->SetRelativeOffsetInternal(position);
    }
}

void UiElement::SetRelativeOffsetInternal(Vector2 offset)
{
    if (offset != _positioning.relativeOffset.XY())
    {
        _positioning.relativeOffset = _positioning.relativeOffset.SetXY(offset);
        MarkAbsolutePositionDirty();
    }
}
