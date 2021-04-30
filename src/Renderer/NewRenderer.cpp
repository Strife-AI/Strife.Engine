#include "NewRenderer.hpp"
#include "Resource/ResourceManager.hpp"
#include "Renderer/Texture.hpp"
#include "Resource/ShaderResource.hpp"

void LayeredRenderStage::Execute()
{
    Effect* lastEffect = nullptr;
    RendererState rendererState;
    for (auto layer : layers)
    {
        for (auto& effectPair : layer->renderables)
        {
            Effect* effect = effectPair.first;
            if (effect != lastEffect)
            {
                if (lastEffect != nullptr)
                {
                    lastEffect->Stop();
                    lastEffect->Flush();
                }

                effect->renderer = &rendererState;
                effect->Start();
                lastEffect = effect;
            }

            for (IRenderable* renderable : effectPair.second)
            {
                renderable->Render();
            }
        }
    }

    if (lastEffect != nullptr)
    {
        lastEffect->Flush();
        lastEffect->Stop();
    }
}