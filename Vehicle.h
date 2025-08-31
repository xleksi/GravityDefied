#pragma once
#include <raylib.h>
#include "Wheel.h"

class Vehicle {
public:
    Vector2 position;
    Vector2 velocity;
    float width;
    float height;
    float angle;
    Vector2 spriteOffset;

    Wheel backWheel;
    Wheel frontWheel;

    Vehicle();

    void PlaceAt(float x, float y);
    void Control(float dt);
    void MoveBody(float dt);
    void Rotate(float dt);
    void Draw(Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw = true);
};
