#pragma once

#include "BaseEntity.hpp"

DEFINE_ENTITY(AmbientLightEntity, "ambient-light")
{
    void OnAdded() override;
    void OnDestroyed() override;

    AmbientLight ambientLight;
};