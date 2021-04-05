#pragma once
#include "Scene/Scene.hpp"

class PlayerCommandRunner : public ISceneService
{
public:
    PlayerCommandRunner(bool isServer)
        : _isServer(isServer)
    {
        
    }

private:
    void ReceiveEvent(const IEntityEvent& ev) override;
    void ServerFixedUpdate();

    bool _isServer;
    int currentFixedUpdateId = 0;
};
