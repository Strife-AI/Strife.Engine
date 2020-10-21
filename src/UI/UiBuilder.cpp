#include "UiDictionary.hpp"
#include "UI.hpp"
#include "nlohmann/json.hpp"

struct UiElementDto
{
    UiElementDto()
        : dictionary(std::make_shared<UiDictionary>())
    {
        
    }

    UiElementDto(UiElementDto&&) = default;

    std::shared_ptr<UiDictionary> dictionary;
    std::vector<std::shared_ptr<UiElementDto>> children;
};

std::unique_ptr<UiDictionary> ParseObjectRecursive(nlohmann::json& j)
{
    auto dictionary = std::make_unique<UiDictionary>();

    for (auto property : j.items())
    {
        if (property.value().is_object())
        {
            auto subDictionary = ParseObjectRecursive(property.value());
            dictionary->AddProperty(property.key().c_str(), subDictionary);
        }
        else if(property.value().is_number())
        {
            auto str = std::to_string(property.value().get<double>());
            dictionary->AddProperty(property.key().c_str(), str);
        }
        else
        {
            auto str = property.value().get<std::string>();
            dictionary->AddProperty(property.key().c_str(), str);
        }
    }

    return dictionary;
}

std::unique_ptr<UiElementDto> ParseElementRecursive(nlohmann::json& j)
{
    auto element = std::make_unique<UiElementDto>();

    for (auto property : j.items())
    {
        if (property.value().is_object())
        {
            auto subDictionary = ParseObjectRecursive(property.value());
            element->dictionary->AddProperty(property.key().c_str(), subDictionary);
        }
        else if(property.value().is_array() && property.key() == "children")
        {
            for(auto childJson : property.value().items())
            {
                auto child = ParseElementRecursive(childJson.value());
                element->children.emplace_back(std::move(child));
            }
        }
        else if (property.value().is_number())
        {
            auto str = std::to_string(property.value().get<double>());
            element->dictionary->AddProperty(property.key().c_str(), str);
        }
        else
        {
            auto str = property.value().get<std::string>();
            element->dictionary->AddProperty(property.key().c_str(), str);
        }
    }

    return element;
}

static UiDictionary g_emptyDictionary;

template<>
bool UiDictionary::TryGetCustomProperty(const char* key, PaddingStyle& outResult) const
{
    if(!HasProperty(key))
    {
        return false;
    }

    float padding;
    if (TryGetProperty(key, padding))
    {
        outResult.left = outResult.right = outResult.top = outResult.bottom = padding;
        return true;
    }
    else
    {
        UiDictionary* style = GetValueOrDefault(key, &g_emptyDictionary);

        outResult.left = style->GetValueOrDefault("left", 0);
        outResult.right = style->GetValueOrDefault("right", 0);
        outResult.top = style->GetValueOrDefault("top", 0);
        outResult.bottom = style->GetValueOrDefault("bottom", 0);

        return true;
    }
}

template<>
bool UiDictionary::TryGetCustomProperty(const char* key, LabelStyle& outResult) const
{
    UiDictionary* style = GetValueOrDefault(key, &g_emptyDictionary);

    outResult.font.scale = style->GetValueOrDefault("scale", 1.0);
    std::string_view fontName = style->GetValueOrDefault("font", "console-font");

    outResult.font.spriteFont = ResourceManager::GetResource<SpriteFont>(StringId(fontName));
    outResult.text = style->GetValueOrDefault("text", "LABEL");

    return true;
}

template<>
bool UiDictionary::TryGetCustomProperty(const char* key, BackgroundStyle& outResult) const
{
    UiDictionary* style = GetValueOrDefault(key, &g_emptyDictionary);

    UiDictionary* sprite;
    Color color;

    auto resultFromSprite = [=, &outResult](StringId name)
    {
        Resource<Sprite> spriteResource;
        Resource<NineSlice> sliceResource;

        // TODO: color blending

        if (ResourceManager::TryGetResource(name, spriteResource))
        {
            outResult.type = BackgroundType::Sprite;
            outResult.sprite = spriteResource;
        }
        else if (ResourceManager::TryGetResource(name, sliceResource))
        {
            outResult.type = BackgroundType::NineSlice;
            outResult.nineSlice = sliceResource;
        }
        else
        {
            outResult.type = BackgroundType::Colored;
            outResult.color = Color(255, 0, 0);
        }
    };

    std::string_view spriteName;

    if(TryGetProperty(key, color))
    {
        outResult.type = BackgroundType::Colored;
        outResult.color = Color(255, 0, 0);
    }
    else if(TryGetProperty(key, spriteName))
    {
        resultFromSprite(StringId(spriteName));
    }
    else if(style->TryGetProperty("sprite", sprite))
    {
        auto name = StringId(sprite->GetValueOrDefault("name", "level-select-panel.9"));
        resultFromSprite(name);
    }
    else if(style->TryGetProperty("color", color))
    {
        outResult.type = BackgroundType::Colored;
        outResult.color = color;
    }
    else
    {
        outResult.type = BackgroundType::Colored;
        outResult.color = Color(255, 0, 0);
    }

    return true;
}

UiElementPtr BuildLabel(const UiDictionary& properties)
{
    auto style = std::make_shared<LabelStyle>();
    properties.TryGetProperty("style", *style);

    auto label = std::make_shared<Label>();
    label->SetStyle(style);

    return label;
}

UiElementPtr BuildButton(const UiDictionary& properties)
{
    auto button = std::make_shared<Button>();

    auto buttonStyle = std::make_shared<ButtonStyles>();

    UiDictionary* style = properties.GetValueOrDefault("style", &g_emptyDictionary);

    buttonStyle->pressedStyle = std::make_shared<BackgroundStyle>();
    style->GetProperty("pressed", *buttonStyle->pressedStyle);

    buttonStyle->releasedStyle = std::make_shared<BackgroundStyle>();
    style->GetProperty("released", *buttonStyle->releasedStyle);

    buttonStyle->hoverStyle = std::make_shared<BackgroundStyle>();
    style->GetProperty("hover", *buttonStyle->hoverStyle);

    PaddingStyle labelPadding;
    if(style->TryGetProperty("padding", labelPadding))
    {
        button->SetInternalPadding(labelPadding);
    }

    button->SetStyle(buttonStyle);

    if (properties.HasProperty("label"))
    {
        auto labelStyle = std::make_shared<LabelStyle>();
        properties.GetProperty("label", *labelStyle);

        button->UseLabel(labelStyle);
    }

    return button;
}

UiElementPtr BuildStackedPanel(const UiDictionary& properties)
{
    auto panel = std::make_shared<StackedLayoutPanel>();

    std::string_view orientation;
    if(properties.TryGetProperty("orientation", orientation))
    {
        panel->SetOrientation(orientation == "vertical" ? StackedLayoutDimension::Vertical : StackedLayoutDimension::Horizontal);
    }

    float spacing = properties.GetValueOrDefault("spacing", 0);

    panel->SetSpacing(spacing);

    return panel;
}

UiElementPtr BuildUiRecrusive(UiElementDto* uiElementDto)
{
    auto& properties = *uiElementDto->dictionary;

    std::string_view type;
    if(!properties.TryGetProperty("type", type))
    {
        FatalError("Missing element type");
    }

    UiElementPtr element;

    if(type == "label")
    {
        element = BuildLabel(properties);
    }
    else if(type == "panel")
    {
        element = std::make_shared<UiElement>();
    }
    else if(type == "button")
    {
        element = BuildButton(properties);
    }
    else if(type == "stacked")
    {
        element = BuildStackedPanel(properties);
    }
    else
    {
        FatalError("Unknown element type %s\n", std::string(type).c_str());
    }

    element->SetRelativePosition(properties.GetValueOrDefault("offset", Vector2(0, 0)));

    for(auto& childDto : uiElementDto->children)
    {
        auto child = BuildUiRecrusive(childDto.get());
        element->AddExistingChild(child);
    }

    if(properties.HasProperty("background"))
    {
        auto background = std::make_shared<BackgroundStyle>();
        properties.GetProperty("background", *background);

        element->SetBackgroundStyle(background);
    }

    int align = 0;
    std::string_view alignX;
    std::string_view alignY;

    if(properties.TryGetProperty("align-x", alignX))
    {
        if (alignX == "left") align |= AlignLeft;
        else if (alignX == "right") align |= AlignRight;
        else if (alignX == "center") align |= AlignCenterX;
    }

    if (properties.TryGetProperty("align-y", alignY))
    {
        if (alignY == "top") align |= AlignTop;
        else if (alignY == "bottom") align |= AlignBottom;
        else if (alignY == "center") align |= AlignCenterY;
    }

    element->SetAlign(align);

    PaddingStyle padding;

    if(properties.TryGetProperty("padding", padding))
    {
        auto panel = std::make_shared<PaddingPanel>();
        panel->SetPadding(padding);

        panel->AddExistingChild(element);

        return panel;
    }
    else
    {
        return element;
    }
}

std::shared_ptr<UiCanvas> DeserializeUi(const std::string& jsonString)
{
    auto canvas = std::make_shared<UiCanvas>();
    auto json = nlohmann::json::parse(jsonString);

    auto uiDto = ParseElementRecursive(json);
    auto ui = BuildUiRecrusive(uiDto.get());
    canvas->RootPanel()->AddExistingChild(ui);

    return canvas;
}