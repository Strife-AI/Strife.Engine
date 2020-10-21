#pragma once

#include <Tools/ConsoleVar.hpp>
#include "Entity.hpp"
#include "Renderer/Camera.hpp"
#include "System/Input.hpp"

struct Entity;

extern ConsoleVar<float> g_WasdSpeed;

enum class CameraFollowMode
{
    None,
    Entity,
    Mouse
};

class CameraFollower
{
public:
    CameraFollower(Camera* camera, Input* input)
        : _camera(camera),
        _input(input)
    {
        
    }


    CameraFollowMode Mode() const { return _mode; }
    Entity* GetFollowedEntity() const
    {
        return _mode == CameraFollowMode::Entity ? _followEntity.GetValueOrNull() : nullptr;
    }

    void FollowEntity(Entity* entity);
    void FollowMouse();
    void Detach();
    void CenterOn(Vector2 position);
    void Update(float deltaTime);
    void SetMaxX(float x);
    void SetMinX(float x);
    void SetFollowOffset(Vector2 offset)
    {
        _followOffset = offset;
    }

    float shakeTarget = 0;
    float shake = 0;

private:
    Rectangle TargetBounds();
    bool ApplyContraints();

    Camera* _camera;
    Input* _input;
    EntityReference<Entity> _followEntity;
    Vector2 _followOffset;
    CameraFollowMode _mode = CameraFollowMode::None;
    float _maxSpeed = 100;
    Vector2 _buffer = Vector2(400, 200) * 2;
    Vector2 _velocity = Vector2(0, 0);

    float _maxX = 1e9;
    float _minX = 0;
    float _fixConstraintSpeed = 1000;

    float _shakeVelocity = 0;
};
