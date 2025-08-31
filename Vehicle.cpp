#include "Vehicle.h"
#include <raymath.h>
#include <cmath>
#include <raylib.h>
#include "Config.h"
#include "Utils.h"


Vehicle::Vehicle() {
    position = {1200, 300};
    width = 250;
    height = 100;
    angle = 0;
    spriteOffset = {75.5f, 52.5f};

    // Back wheel
    backWheel.radius = 32.5f;
    backWheel.padding = 10;
    backWheel.stiffness = 1.2f;
    backWheel.damping = 2.0f;
    backWheel.offset = 20;
    backWheel.spriteOffset = {32.5f, 35};
    backWheel.spriteScale = 1.1f;
    backWheel.position = {position.x - width / 2 + backWheel.radius + backWheel.padding,
                          position.y + height / 2 + backWheel.radius + backWheel.padding};
    backWheel.lastX = backWheel.position.x;

    // Front wheel
    frontWheel.radius = 32.5f;
    frontWheel.padding = 10;
    frontWheel.stiffness = 1.2f;
    frontWheel.damping = 2.0f;
    frontWheel.offset = 175;
    frontWheel.spriteOffset = {32.5f, 35};
    frontWheel.spriteScale = 1.1f;
    frontWheel.position = {position.x + width / 2 - frontWheel.radius - frontWheel.padding,
                           position.y + height / 2 + frontWheel.radius + frontWheel.padding};
    frontWheel.lastX = frontWheel.position.x;
}

void Vehicle::Control(float dt) {
    float maxTilt = 8.0f;
    bool accelerating = IsKeyDown(KEY_RIGHT);
    bool braking = IsKeyDown(KEY_LEFT);

    const float MAX_SPEED = 10.0f;

    if (accelerating) {
        float torque = 4.0f * dt;
        velocity.x += torque;
        backWheel.velocity.x += torque;
        if (velocity.x > MAX_SPEED) velocity.x = MAX_SPEED;
    }
    else if (braking) {
        velocity.x -= 4.0f * 3.0f * dt;
        if (velocity.x < 0.0f) velocity.x = 0.0f;

        float targetTilt = maxTilt;
        float diff = targetTilt - angle;
        angle += diff * 6.0f * dt;
    }
    else {
        velocity.x -= velocity.x * 0.6f * dt; // friction
        if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    }
}

void Vehicle::Rotate(float dt) {
    if (backWheel.on_ground && frontWheel.on_ground) {
        float target = Vector2LineAngle(backWheel.position, frontWheel.position) * RAD2DEG;
        target = -target;

        float diff = target - angle;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff <= -180.0f) diff += 360.0f;

        angle += diff * 3.0f * dt;

        float tilt = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) tilt = -velocity.x * 2.0f;
        else if (IsKeyDown(KEY_LEFT)) tilt = 20.0f;

        angle += tilt * dt;
    }
}

void Vehicle::Move(const std::vector<Vector2>& terrain, float dt, bool debugDraw) {
    position.x += velocity.x;
    position.y += velocity.y;

    backWheel.Move(terrain, dt);
    frontWheel.Move(terrain, dt);

    backWheel.ApplySuspension(position, angle, velocity, height, width, dt);
    frontWheel.ApplySuspension(position, angle, velocity, height, width, dt);

    backWheel.UpdateRotation();
    frontWheel.UpdateRotation();

    float maxY = GetScreenHeight() - height / 2;
    if (position.y >= maxY) {
        velocity.y = 0;
        position.y = maxY;
    } else {
        velocity.y += 5 * dt; // gravity
    }
}

void Vehicle::Draw(Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw) {
    // Draw wheels
    backWheel.Draw(wheelRearTex, debugDraw);
    frontWheel.Draw(wheelFrontTex, debugDraw);

    // Draw driver head
    if (debugDraw) {
        Vector2 headPos = {position.x - 42.5f + spriteOffset.x,
                           position.y - height * 0.65f + spriteOffset.y};
        DrawCircleV(headPos, 15, Color{0, 0, 255, 50});
        DrawCircleLines((int)headPos.x, (int)headPos.y, 15, BLUE);
        Vector2 textSize = MeasureTextEx(GetFontDefault(), "HEAD", 14, 1);
        DrawText("HEAD", headPos.x - textSize.x / 2, headPos.y - textSize.y / 2, 14, BLACK);
    }

    // Draw vehicle body
    Rectangle srcBody = {0.0f, 0.0f, (float)bodyTex.width, (float)bodyTex.height};
    Rectangle destBody = {position.x - width / 2 + spriteOffset.x,
                          position.y - height / 2 + spriteOffset.y,
                          width, height};
    Vector2 originBody = {width / 2 - spriteOffset.x, height / 2 - spriteOffset.y};
    DrawTexturePro(bodyTex, srcBody, destBody, originBody, angle, WHITE);
}
