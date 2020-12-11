#pragma once
#include "UI.hpp"
#include "UiElement.hpp"

std::shared_ptr<Button> FlatTextButton(const std::string& text, Color background)
{
    auto button = std::make_shared<Button>();
    button->SetStyle(style.buttonStyle);

    button
        ->AddChild<PaddingPanel>()
        ->SetPadding(style.padding, style.padding, style.padding, style.padding)
        ->SetAlign(AlignCenterX | AlignCenterY)
        ->AddChild<Label>()
        ->SetStyle(style.labelStyle)
        ->SetText(text);

    return button;
}

