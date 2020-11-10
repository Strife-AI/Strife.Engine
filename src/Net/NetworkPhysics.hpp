#pragma once
#include "Scene/Scene.hpp"

class NetworkPhysics : public ISceneService
{
public:
    NetworkPhysics(bool isServer)
        : _isServer(isServer)
    {
        
    }

    void UpdateClientPrediction(NetComponent* self);

private:
    void ReceiveEvent(const IEntityEvent& ev) override;
    void ServerFixedUpdate();
    void ClientFixedUpdate();

    bool _isServer;
    int currentFixedUpdateId = 0;
};
