#pragma once
#include "raylib.h"
#include "Collision.h"

class Player {
public:
    Vector2 position;
    Vector2 velocity;
    bool canJump;

    Circle rearWheel;
    Circle frontWheel;
    // Additional collision shapes
    Circle head;
    Polygon bodyPoly;

    // Visuals
    Texture2D bodyTexture;
    Texture2D wheelFrontTexture;
    Texture2D wheelRearTexture;

    // Starting X position for distance tracking
    float startX;

    Player(Vector2 startPos);

    void Update(float delta, const Rectangle& ground);
    void Draw() const;
    void Reset(Vector2 startPos);
    float GetDistance() const;

private:
    void UpdateCollisionShapes();
};
