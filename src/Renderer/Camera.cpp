#include "Camera.hpp"
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>


#include "Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneManager.hpp"
#include "Tools/Console.hpp"
#include "Tools/ConsoleCmd.hpp"

using namespace  glm;

void Camera::RebuildViewMatrix()
{
    Vector2 pos = Vector2(round(Position().x), round(Position().y));

    mat4 translation = translate(mat4(1), vec3(-pos.x + ScreenSize().x / 2, -pos.y + ScreenSize().y / 2, 0));
    mat4 scale = glm::scale(mat4x4(1), vec3(2.0f / _viewport.x, 2.0f / _viewport.y, 1));
    mat4 centerInScreen = translate(mat4(1), vec3(-1, -1, 0));
    mat4 zoom = glm::scale(mat4x4(1), vec3(_zoom, -_zoom, 1.0f));

    _viewMatrix = zoom * centerInScreen * scale * translation;
}

Vector2 Camera::ScreenToWorld(Vector2 screenPoint) const
{
    return (screenPoint - _viewport / 2) / _zoom + _position;
}

Vector2 Camera::WorldToScreen(Vector2 worldPoint) const
{
    return (worldPoint - _position) * _zoom + _viewport / 2;
}

void Camera::SetScreenSize(Vector2 size)
{
    _viewport = size;
}

void Camera::SetPosition(Vector2 position)
{
    _position = position;
}

void Camera::SetZoom(float zoom)
{
    _zoom = floor(zoom * 8) / 8;
}

void CameraCommand(ConsoleCommandBinder& binder)
{
    binder.Help("Shows info about camera");



    //auto position = Engine::GetInstance()->GetSceneManager()->GetScene()->GetCamera()->Position();
    //auto bounds = Engine::GetInstance()->GetSceneManager()->GetScene()->GetCamera()->Bounds();

    //binder.GetConsole()->Log("Position: %f %f\n", position.x, position.y);
    //binder.GetConsole()->Log("TopLeft: %f %f\n", bounds.TopLeft().x, bounds.TopLeft().y);
    //binder.GetConsole()->Log("BottomRight: %f %f\n", bounds.BottomRight().x, bounds.BottomRight().y);

}
ConsoleCmd cameraCmd("camera", CameraCommand);