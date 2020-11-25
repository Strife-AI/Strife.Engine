#pragma once
#include "Scene/Scene.hpp"

class NetworkPhysics : public ISceneService
{
public:
    NetworkPhysics(bool isServer)
        : _isServer(isServer)
    {
        
    }

private:
    void ReceiveEvent(const IEntityEvent& ev) override;
    void ServerFixedUpdate();

    bool _isServer;
    int currentFixedUpdateId = 0;
};
