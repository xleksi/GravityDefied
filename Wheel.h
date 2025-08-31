#pragma once
#include <raylib.h>
#include <vector>

class Wheel {
public:
    Vector2 position;
    Vector2 velocity;
    float radius;
    float padding;
    float stiffness;
    float damping;
    float offset;
    float attachOffsetY;
    Vector2 spriteOffset;
    float spriteScale;
    bool on_ground;
    float visualRotation;
    float lastX;

    Wheel(float r = 32.5f);

    void Move(const std::vector<Vector2>& terrain, float dt);
    void UpdateRotation();
    void ApplySuspension(const Vector2& vehiclePos, float vehicleAngle, Vector2& vehicleVelocity, float vehicleHeight, float vehicleWidth, float dt);
    void Draw(Texture2D tex, bool debugDraw = true);
};
