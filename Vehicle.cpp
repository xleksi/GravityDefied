#include "Vehicle.h"
#include "Config.h"
#include "Utils.h"
#include <raymath.h>
#include <cmath>

// struct
Vehicle::Vehicle() {
    position = {1200.0f, 300.0f};
    velocity = {0.0f, 0.0f};
    width = 250.0f;
    height = 100.0f;
    angle = 0.0f;
    spriteOffset = {75.5f, 52.5f};

    // Back wheel
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

    // Front wheel
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

void Vehicle::PlaceAt(float x, float y) {
    position.x = x;
    position.y = y;
    velocity = {0.0f, 0.0f};
    angle = 0.0f;

    Vector2 bottomDir = Vector2Rotate({0, 1}, angle * DEG2RAD);

    // BACK WHEEL
    {
        Vector2 attachLocal = {-width * 0.5f + backWheel.padding + backWheel.radius + backWheel.offset,
                               backWheel.attachOffsetY};
        Vector2 attachWorld = Vector2Add(Vector2Rotate(attachLocal, angle * DEG2RAD), position);

        // resting length = body half-height + padding + wheel radius
        float restingLen = height * 0.5f + backWheel.padding + backWheel.radius;

        // to place wheel at the resting pos
        backWheel.position = Vector2Add(attachWorld, Vector2Scale(bottomDir, restingLen));

        backWheel.velocity = velocity;
        backWheel.lastX = backWheel.position.x;
        backWheel.on_ground = false;
        backWheel.visualRotation = backWheel.visualRotation;
    }

    // FRONT WHEEL
    {
        Vector2 attachLocal = { width * 0.5f - frontWheel.radius - frontWheel.padding + frontWheel.offset,
                                frontWheel.attachOffsetY};
        // front wheel same as rear calculation but mirrored horizontally.
        Vector2 attachLocalCorrect = {-width * 0.5f + frontWheel.padding + frontWheel.radius + frontWheel.offset,
                                      frontWheel.attachOffsetY};
        Vector2 attachWorld = Vector2Add(Vector2Rotate(attachLocalCorrect, angle * DEG2RAD), position);

        float restingLen = height * 0.5f + frontWheel.padding + frontWheel.radius;
        frontWheel.position = Vector2Add(attachWorld, Vector2Scale(bottomDir, restingLen));
        frontWheel.velocity = velocity;
        frontWheel.lastX = frontWheel.position.x;
        frontWheel.on_ground = false;
        frontWheel.visualRotation = frontWheel.visualRotation;
    }
}

//acceleration / braking and tilt
void Vehicle::Control(float dt) {
    float maxTilt = 8.0f;
    bool accelerating = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    bool braking      = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    const float MAX_SPEED = 10.0f;

    if (accelerating) {
        float torque = VEHICLE_SPEED * dt;
        velocity.x += torque;
        backWheel.velocity.x += torque;
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
        // friction slows vehicle
        velocity.x -= velocity.x * FRICTION * dt;
        if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    }
}

void Vehicle::MoveBody(float dt) {
    position.x += velocity.x;
    position.y += velocity.y;

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

// Rotate body to follow slope
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

void Vehicle::Draw(Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw) {
    // Draw wheels
    backWheel.Draw(wheelRearTex, debugDraw);
    frontWheel.Draw(wheelFrontTex, debugDraw);

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
