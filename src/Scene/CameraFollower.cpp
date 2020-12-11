#include "CameraFollower.hpp"

#include <SDL_scancode.h>

#include "Renderer.hpp"
#include "Math/Random.hpp"

ConsoleVar<float> g_WasdSpeed("wasd-speed", 1000.0f, false);

void CameraFollower::FollowEntity(Entity* entity)
{
    _followEntity = entity;
    _mode = CameraFollowMode::Entity;
    _velocity = { 0, 0 };
}

void CameraFollower::FollowMouse()
{
    _mode = CameraFollowMode::Mouse;
}

void CameraFollower::Detach()
{
    _mode = CameraFollowMode::None;
}

void CameraFollower::CenterOn(Vector2 position)
{
    _camera->SetPosition(position);
    ApplyContraints();
    _velocity = { 0, 0 };
}

void CameraFollower::Update(float deltaTime)
{
    if(_mode == CameraFollowMode::None)
    {
        return;
    }
    else if (_mode == CameraFollowMode::Mouse)
    {
        if (_input->GetMouse()->MiddlePressed())
        {
            _camera->SetPosition(_camera->Position() + _input->GetMouse()->MousePosition() - _camera->ScreenSize() / 2);
        }

        Vector2 offset;
        float wasdSpeed = g_WasdSpeed.Value();

        if (InputButton(ButtonMapping::Key(SDL_SCANCODE_W)).IsDown()) offset += Vector2(0, -1);
        else if (InputButton(ButtonMapping::Key(SDL_SCANCODE_S)).IsDown()) offset += Vector2(0, 1);

        if (InputButton(ButtonMapping::Key(SDL_SCANCODE_A)).IsDown()) offset += Vector2(-1, 0);
        else if (InputButton(ButtonMapping::Key(SDL_SCANCODE_D)).IsDown()) offset += Vector2(1, 0);

        float scroll = _input->MouseWheelY();

        Vector2 change = (InputButton(ButtonMapping::Key(SDL_SCANCODE_LCTRL)).IsDown() ? Vector2(1, 0) : Vector2(0, -1))
            * scroll * 256;

        _camera->SetPosition(_camera->Position() + change + offset * wasdSpeed * deltaTime);

        return;
    }
    else if (_mode == CameraFollowMode::Entity)
    {
        auto buffer = _buffer * _camera->Zoom() / 2;

        auto topLeft = _camera->ScreenToWorld({ 0, buffer.y});
        auto bottomRight = _camera->ScreenToWorld(_camera->ScreenSize() - Vector2(0, buffer.y));

        Rectangle cameraScreenBounds(
            topLeft,
            bottomRight - topLeft);

        auto targetBounds = Rectangle(TargetBounds().GetCenter(), Vector2(0, 0));
        auto offset = targetBounds.AlignInsideOf(cameraScreenBounds);

        auto newTargetPosition = Vector2(
            TargetBounds().GetCenter().x,  
            (_camera->Position() - offset).y) + _followOffset * _camera->Zoom() / 2;

        auto newPosition = _camera->Position().SmoothDamp(
            newTargetPosition,
            _velocity,
            0.3f,
            deltaTime);

        if (shake != 0 && deltaTime != 0)
        {
            newPosition += Rand(Vector2(-shake, -shake), Vector2(shake, shake));
        }

        shake = SmoothDamp(shake, shakeTarget, _shakeVelocity, 0.3, deltaTime);

        _camera->SetPosition(newPosition);

        ApplyContraints();
    }
}

void CameraFollower::SetMaxX(float x)
{
    _maxX = x;
}

void CameraFollower::SetMinX(float x)
{
    _minX = x;
    //ApplyContraints();
}

Rectangle CameraFollower::TargetBounds()
{
    switch (_mode)
    {
    case CameraFollowMode::Entity:
    {
        Entity* entity;
        return _followEntity.TryGetValue(entity)
            ? entity->Bounds()
            : Rectangle(_camera->Position(), Vector2(0, 0));
    }
    case CameraFollowMode::Mouse:
        return Rectangle(_camera->Position() + _input->GetMouse()->MousePosition(), Vector2(0, 0));

    default:
        return Rectangle(_camera->Position(), Vector2(0, 0));
    }
}

bool CameraFollower::ApplyContraints()
{
    if(_camera->Bounds().Left() < _minX)
    {
        float offset = _minX - _camera->Bounds().Left();
        _camera->SetPosition(_camera->Position() + Vector2(offset, 0));

        if (_velocity.x < 0) _velocity.x = 0;
    }

    return false;
}
