#pragma once

#include "Scene/EntityComponent.hpp"
#include "Net/SyncVar.hpp"

DEFINE_COMPONENT(HealthBarComponent)
{
    void Render(Renderer* renderer) override;
    void TakeDamage(int amount);

    SyncVar<uint16_t> health {100 };
    Vector2 offsetFromCenter;
    int maxHealth = 100;
};