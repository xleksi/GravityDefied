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

    // Place body (and initialize wheel positions + lastX)
    void PlaceAt(float x, float y);

    // Body-only control & movement
    void Control(float dt);
    void MoveBody(float dt); // move body + apply gravity + simple wheel-based friction
    void Rotate(float dt);

    // Drawing
    void Draw(Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw = true);
};
