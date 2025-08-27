#pragma once
#include "raylib.h"
#include "Collision.h"
#include "Terrain.h"

class Player {
public:
    Vector2 position;
    Vector2 velocity;

    Circle rearWheel;
    Circle frontWheel;
    Circle head;
    Polygon bodyPoly;
    
    float acceleration = 0.0f;
    float wheelRotationRear = 0.0f;
    float wheelRotationFront = 0.0f;
    float scale = 0.33f;
    float bodyRotation = 0.0f;

    Texture2D bodyTexture;
    Texture2D wheelFrontTexture;
    Texture2D wheelRearTexture;

    float startX;

    Player(Vector2 startPos);

    void Update(float delta, const Terrain& terrain);
    void Draw() const;
    void Reset(Vector2 startPos);
    float GetDistance() const;

private:
    void UpdateCollisionShapes();
};
