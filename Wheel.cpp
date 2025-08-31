#include "Wheel.h"
#include "Utils.h"
#include <raymath.h>
#include <cmath>
#include "Config.h"
#include "Utils.h"


Wheel::Wheel(float r)
    : radius(r), padding(10), stiffness(1.2f), damping(2.0f),
      offset(0), attachOffsetY(0), spriteOffset({r, r}),
      spriteScale(1.0f), on_ground(false), visualRotation(0), lastX(0) {}

void Wheel::Move(const std::vector<Vector2>& terrain, float dt) {
    position.x += velocity.x;
    position.y += velocity.y;
    on_ground = false;

    if (terrain.size() >= 2) {
        for (size_t i = 1; i < terrain.size(); i++) {
            Vector2 collision_point = {0};
            Vector2 bottom_of_wheel = {position.x, position.y + radius};
            if (IsPointBelowLine(terrain[i - 1], terrain[i], bottom_of_wheel, &collision_point)) {
                velocity.y = 0;
                position.y = collision_point.y - radius + 1;
                on_ground = true;
                break;
            }
        }
    }

    if (!on_ground) velocity.y += 5 * dt; // GRAVITY
}

void Wheel::UpdateRotation() {
    float dx = position.x - lastX;
    visualRotation += (dx / (2.0f * PI * radius)) * 360.0f;
    lastX = position.x;
}

void Wheel::ApplySuspension(const Vector2& vehiclePos, float vehicleAngle, Vector2& vehicleVelocity, float vehicleHeight, float vehicleWidth, float dt) {
    Vector2 bottom_direction = Vector2Rotate({0, 1}, vehicleAngle * DEG2RAD);
    Vector2 attachment_point_local = {-vehicleWidth / 2 + padding + radius + offset, attachOffsetY};
    Vector2 attachment_point = Vector2Rotate(attachment_point_local, vehicleAngle * DEG2RAD);
    attachment_point = Vector2Add(attachment_point, vehiclePos);

    float length = Vector2Distance(position, attachment_point);
    float resting_length = vehicleHeight / 2 + padding + radius;
    float stretch = length - resting_length;

    position = Vector2Add(attachment_point, Vector2Scale(bottom_direction, length));

    float spring_force = stretch * stiffness * dt;
    Vector2 relative_velocity = Vector2Subtract(vehicleVelocity, velocity);
    Vector2 damping_force = Vector2Scale(relative_velocity, damping * dt);
    Vector2 force = Vector2Subtract(Vector2Scale(bottom_direction, spring_force), damping_force);

    vehicleVelocity = Vector2Add(vehicleVelocity, force);
    velocity = Vector2Subtract(velocity, Vector2Scale(force, 0.7f));
}

void Wheel::Draw(Texture2D tex, bool debugDraw) {
    float diameter = radius * 2.0f;
    float texAspect = (float)tex.height / (float)tex.width;
    Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
    Rectangle dest = {position.x - diameter * 0.5f + spriteOffset.x,
                      position.y - diameter * 0.5f + spriteOffset.y,
                      diameter * spriteScale, diameter * spriteScale * texAspect};
    Vector2 origin = {dest.width * 0.5f, dest.height * 0.5f};
    DrawTexturePro(tex, src, dest, origin, visualRotation, WHITE);

    if (debugDraw) DrawCircleLines((int)position.x, (int)position.y, radius, RED);
}