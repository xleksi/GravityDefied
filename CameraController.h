#pragma once
#include "raylib.h"
#include "Player.h"

class CameraController 
{
    public:
        Camera2D camera;

    CameraController(Vector2 target, int screenWidth, int screenHeight);
    void Update(const Player& player, float delta);
};
