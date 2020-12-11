#pragma once

#include "BaseEntity.hpp"

DEFINE_ENTITY(AmbientLightEntity, "ambient-light")
{
    void OnAdded(const EntityDictionary & properties) override;
    void OnDestroyed() override;

    AmbientLight ambientLight;
};