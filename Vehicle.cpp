#include "Vehicle.h"
#include "Config.h"
#include "Utils.h"
#include <raymath.h>
#include <cmath>

// Constructor: set sensible defaults for wheels and body
Vehicle::Vehicle() {
    position = {1200.0f, 300.0f};
    velocity = {0.0f, 0.0f};
    width = 250.0f;
    height = 100.0f;
    angle = 0.0f;
    spriteOffset = {75.5f, 52.5f};

    // Back wheel defaults
    backWheel.radius = 32.5f;
    backWheel.padding = 10.0f;
    backWheel.stiffness = 1.2f;
    backWheel.damping = 2.0f;
    backWheel.offset = 20.0f;
    backWheel.attachOffsetY = 0.0f;
    backWheel.spriteOffset = {32.5f, 35.0f};
    backWheel.spriteScale = 1.1f;
    backWheel.on_ground = false;
    backWheel.visualRotation = 0;
    backWheel.lastX = 0;

    // Front wheel defaults
    frontWheel.radius = 32.5f;
    frontWheel.padding = 10.0f;
    frontWheel.stiffness = 1.2f;
    frontWheel.damping = 2.0f;
    frontWheel.offset = 175.0f;
    frontWheel.attachOffsetY = 0.0f;
    frontWheel.spriteOffset = {32.5f, 35.0f};
    frontWheel.spriteScale = 1.1f;
    frontWheel.on_ground = false;
    frontWheel.visualRotation = 0;
    frontWheel.lastX = 0;
}

// Place vehicle at world pos and initialize wheels (positions + lastX + reset velocities)
void Vehicle::PlaceAt(float x, float y) {
    position.x = x;
    position.y = y;
    velocity = {0.0f, 0.0f};
    angle = 0.0f;

    // Position wheels relative to body (visual/physics start positions)
    backWheel.position.x = position.x - (width * 0.5f) + backWheel.radius + backWheel.padding + backWheel.offset;
    backWheel.position.y = position.y + (height * 0.5f) + backWheel.radius + backWheel.padding;
    backWheel.lastX = backWheel.position.x;
    backWheel.velocity = {0.0f, 0.0f};
    backWheel.on_ground = false;

    frontWheel.position.x = position.x + (width * 0.5f) - frontWheel.radius - frontWheel.padding + frontWheel.offset;
    frontWheel.position.y = position.y + (height * 0.5f) + frontWheel.radius + frontWheel.padding;
    frontWheel.lastX = frontWheel.position.x;
    frontWheel.velocity = {0.0f, 0.0f};
    frontWheel.on_ground = false;
}

// Handle player input: acceleration / braking and body tilt
void Vehicle::Control(float dt) {
    float maxTilt = 8.0f;
    bool accelerating = IsKeyDown(KEY_RIGHT);
    bool braking = IsKeyDown(KEY_LEFT);
    const float MAX_SPEED = 10.0f;

    if (accelerating) {
        float torque = VEHICLE_SPEED * dt;
        velocity.x += torque;
        backWheel.velocity.x += torque; // spin rear wheel
        if (velocity.x > MAX_SPEED) velocity.x = MAX_SPEED;
    }
    else if (braking && velocity.x > 0.01f) {
        velocity.x -= VEHICLE_SPEED * 3.0f * dt;
        if (velocity.x < 0.0f) velocity.x = 0.0f;
        float diff = maxTilt - angle;
        angle += diff * 6.0f * dt;
    }
    else if (braking) {
        float diff = maxTilt - angle;
        angle += diff * 3.0f * dt;
    }
    else {
        // No input => friction slows vehicle
        velocity.x -= velocity.x * FRICTION * dt;
        if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    }
}

// Move only body (position), apply gravity and clamp to floor
void Vehicle::MoveBody(float dt) {
    position.x += velocity.x;
    position.y += velocity.y;

    // small wheel-ground friction from previous frame's wheel.on_ground states
    if (backWheel.on_ground || frontWheel.on_ground) {
        float friction = velocity.x * FRICTION;
        velocity.x -= friction * dt;
    }

    float maxY = GetScreenHeight() - height * 0.5f;
    if (position.y >= maxY) {
        position.y = maxY;
        velocity.y = 0;
    } else {
        velocity.y += GRAVITY * dt;
    }
}

// Rotate body to follow slope when both wheels on ground; apply small input tilt
void Vehicle::Rotate(float dt) {
    if (backWheel.on_ground && frontWheel.on_ground) {
        float target = Vector2LineAngle(backWheel.position, frontWheel.position) * RAD2DEG;
        target = -target; // flip for visuals

        float diff = target - angle;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff <= -180.0f) diff += 360.0f;

        angle += diff * ROTATE_BACK_SPEED * dt;

        float tilt = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) tilt = -velocity.x * 2.0f;
        else if (IsKeyDown(KEY_LEFT)) tilt = 20.0f;

        angle += tilt * dt;
    }
}

// Drawing: wheels then body; use sprite offsets for visuals
void Vehicle::Draw(Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw) {
    // Draw wheels
    backWheel.Draw(wheelRearTex, debugDraw);
    frontWheel.Draw(wheelFrontTex, debugDraw);

    // Debug: driver head
    if (debugDraw) {
        Vector2 headPos = { position.x - 42.5f + spriteOffset.x, position.y - height * 0.65f + spriteOffset.y };
        float headRadius = 15.0f;
        DrawCircleV(headPos, headRadius, Color{0, 0, 255, 50});
        DrawCircleLines((int)headPos.x, (int)headPos.y, (int)headRadius, BLUE);
        const char* label = "HEAD";
        int fontSize = 14;
        Vector2 textSize = MeasureTextEx(GetFontDefault(), label, (float)fontSize, 1);
        DrawText(label, (int)(headPos.x - textSize.x * 0.5f), (int)(headPos.y - textSize.y * 0.5f), fontSize, BLACK);
    }

    // Draw body sprite
    Rectangle srcBody = { 0.0f, 0.0f, (float)bodyTex.width, (float)bodyTex.height };
    Rectangle destBody = {
        position.x - width * 0.5f + spriteOffset.x,
        position.y - height * 0.5f + spriteOffset.y,
        width,
        height
    };
    Vector2 originBody = { width * 0.5f - spriteOffset.x, height * 0.5f - spriteOffset.y };
    DrawTexturePro(bodyTex, srcBody, destBody, originBody, angle, WHITE);
}
