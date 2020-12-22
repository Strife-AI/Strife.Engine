#include "AmbientLightEntity.hpp"

void AmbientLightEntity::OnAdded()
{
    scene->GetLightManager()->AddLight(&ambientLight);

#if false
    ambientLight.color = properties.GetValueOrDefault<Color>("color", Color(25, 25, 25));
    ambientLight.intensity = properties.GetValueOrDefault<float>("intensity", 1.0f);
#endif
}

void AmbientLightEntity::OnDestroyed()
{
    scene->GetLightManager()->RemoveLight(&ambientLight);
}
