#include "AmbientLightEntity.hpp"

void AmbientLightEntity::OnAdded(const EntityDictionary& properties)
{
    scene->GetLightManager()->AddLight(&ambientLight);

    ambientLight.color = properties.GetValueOrDefault<Color>("color", Color(25, 25, 25));
    ambientLight.intensity = properties.GetValueOrDefault<float>("intensity", 1.0f);
}

void AmbientLightEntity::OnDestroyed()
{
    scene->GetLightManager()->RemoveLight(&ambientLight);
}
