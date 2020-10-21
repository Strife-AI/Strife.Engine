#pragma once
#include <memory>

class b2World;

namespace Physics
{
    static constexpr float WorldScale = 1.0f / 100.0f;
}

class NewCollider
{
public:

private:

};

class PhysicsManager
{
public:
    NewCollider* CreateCollider();

private:
    std::unique_ptr<b2World> _world;
};