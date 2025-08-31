#pragma once
#include <raylib.h>
#include "Wheel.h"
#include <vector>

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

    void Control(float dt);
    void Rotate(float dt);
    void Move(const std::vector<Vector2>& terrain, float dt, bool debugDraw);
    void Draw(Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw = true);
};
