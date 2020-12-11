#pragma once
#include <box2d/b2_fixture.h>

#include "Math/Rectangle.hpp"

struct RigidBodyComponent;
struct Entity;

class ColliderHandle
{
public:
    ColliderHandle(b2Fixture* fixture)
        : _fixture(fixture)
    {

    }

    ColliderHandle()
        : _fixture(nullptr)
    {
        
    }

    ColliderHandle(const ColliderHandle&) = default;

    Entity* OwningEntity() const;
    RigidBodyComponent* OwningRigidBody() const;

    b2Fixture* GetFixture() const
    {
        return _fixture;
    }

    bool IsTrigger() const
    {
        return _fixture->IsSensor();
    }

    bool operator==(const ColliderHandle& rhs) const
    {
        return _fixture == rhs._fixture;
    }

    Rectangle Bounds() const;

private:
    b2Fixture* _fixture;
};
