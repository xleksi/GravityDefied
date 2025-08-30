#include "CameraController.h"
#include "raymath.h"
#include <cmath>

CameraController::CameraController(const Vector2& target, int screenWidth, int screenHeight)
{
    camera.target = target;
    camera.offset = { (float)screenWidth / 2, (float)screenHeight / 2 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

void CameraController::Update(const Player& player, float delta) 
{
    static float minSpeed = 80;
    static float minEffectLength = 10;
    static float fractionSpeed = 1.5f;

    camera.offset = { GetScreenWidth() / 5.5f, GetScreenHeight() / 2.0f };
    Vector2 diff = Vector2Subtract(player.position, camera.target);
    float length = Vector2Length(diff);

    if (length > minEffectLength) 
    {
        float speed = fmaxf(fractionSpeed * length, minSpeed);
        camera.target = Vector2Add(camera.target, Vector2Scale(diff, speed * delta / length));
    }
}
