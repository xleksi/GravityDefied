#pragma once
#include "raylib.h"
#include "Player.h"

class CameraController {
public:
    Camera2D camera;
    
    // Construct targeting a start position (usually player's pos)
    CameraController(const Vector2& target, int screenWidth, int screenHeight);

    // Smoothly follow the player
    void Update(const Player& player, float deltaTime);
};