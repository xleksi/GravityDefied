#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 1000

#define GRAVITY 7.5f
#define FRICTION 0.6f
#define ROTATION_SPEED 50
#define ROTATE_BACK_SPEED 3
#define VEHICLE_SPEED 5
#define HILL_SPEED 0.4f
#define TRANSPARENT_BLACK (Color){0, 0, 0, 100}

static inline float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

bool IsPointBelowLine(Vector2 a, Vector2 b, Vector2 point, Vector2* collisionPoint) {
    if (fabsf(b.x - a.x) > 1e-6f) {
        float t = (point.x - a.x) / (b.x - a.x);
        if (t < 0.0f || t > 1.0f) return false;

        collisionPoint->x = point.x;
        collisionPoint->y = a.y + t * (b.y - a.y);
        return (point.y > collisionPoint->y);
    } else {
        if (fabsf(point.x - a.x) > 1e-6f) return false;

        collisionPoint->x = a.x;
        collisionPoint->y = fmaxf(a.y, b.y);
        return (point.y > collisionPoint->y);
    }
}

struct Wheel {
    Vector2 position;
    Vector2 velocity;
    float radius;
    float padding;
    float stiffness;
    float damping;
    float offset;             // horizontal attachment offset used by physics
    float attachOffsetY;      // extra vertical attachment tweak (physics)
    Vector2 spriteOffset;     // pixel nudge for sprite when drawing (visual only)
    float spriteScale;        // scale for sprite drawing
    bool on_ground;
    float visualRotation;     // optional: track wheel rotation for sprite
};

struct Vehicle {
    Vector2 position;
    Vector2 velocity;
    float width;
    float height;
    float angle;
    Vector2 spriteOffset;    // pixel nudge for body sprite (visual only)
    Wheel back_wheel;
    Wheel front_wheel;
};

void drawRotatedRectOutline(Vector2 center, float w, float h, float rotationDeg, float thickness, Color col) {
    float rad = rotationDeg * DEG2RAD;
    float c = cosf(rad), s = sinf(rad);
    float hw = w * 0.5f, hh = h * 0.5f;

    Vector2 corners[4] = {
        {-hw, -hh},
        { hw, -hh},
        { hw,  hh},
        {-hw,  hh},
    };

    Vector2 world[4];
    for (int i = 0; i < 4; ++i) {
        world[i].x = center.x + corners[i].x * c - corners[i].y * s;
        world[i].y = center.y + corners[i].x * s + corners[i].y * c;
    }

    for (int i = 0; i < 4; ++i) {
        Vector2 a = world[i];
        Vector2 b = world[(i + 1) % 4];
        DrawLineEx(a, b, thickness, col);
    }
}

void vehicleControl(Vehicle* vehicle, float dt) {
    float maxTilt = 8.0f; // degrees for tilt
    float speedFactor = clampf(fabsf(vehicle->velocity.x) / 10.0f, 0.0f, 1.0f);
    float targetTilt = 0.0f;

    bool accelerating = IsKeyDown(KEY_RIGHT);
    bool braking = IsKeyDown(KEY_LEFT);

    // --- Handle acceleration ---
    if (accelerating) {
        if (vehicle->back_wheel.on_ground) vehicle->velocity.x += VEHICLE_SPEED * dt;
        if (vehicle->front_wheel.on_ground) vehicle->velocity.x += VEHICLE_SPEED * dt;

        // smooth counterclockwise tilt based on speed
        targetTilt = -maxTilt * speedFactor;
        float diff = targetTilt - vehicle->angle;
        vehicle->angle += diff * 2.0f * dt;
    }
    else if (braking && vehicle->velocity.x > 0.01f) {
        // strong braking: just reduce velocity to zero
        vehicle->velocity.x -= VEHICLE_SPEED * 3.0f * dt;
        if (vehicle->velocity.x < 0.0f) vehicle->velocity.x = 0.0f;

        // tilt forward proportional to braking
        targetTilt = maxTilt;
        float diff = targetTilt - vehicle->angle;
        vehicle->angle += diff * 6.0f * dt; // fast tilt
    }
    else if (braking && vehicle->velocity.x <= 0.01f) {
        // vehicle stopped: tilt front slightly
        targetTilt = maxTilt;
        float diff = targetTilt - vehicle->angle;
        vehicle->angle += diff * 3.0f * dt; // smoothing
    }
    else {
        // no input: gradually straighten
        float diff = -vehicle->angle;
        vehicle->angle += diff * 3.0f * dt;
    }

    // --- Apply friction to gradually stop ---
    if (!accelerating && !braking) {
        vehicle->velocity.x -= vehicle->velocity.x * FRICTION * dt;
        if (fabsf(vehicle->velocity.x) < 0.01f) vehicle->velocity.x = 0; // stop completely
    }
}


void vehicleRotate(Vehicle* vehicle, float dt) {
    if (vehicle->back_wheel.on_ground && vehicle->front_wheel.on_ground) {
        // natural rotation along the terrain
        float target = Vector2LineAngle(vehicle->back_wheel.position, vehicle->front_wheel.position) * RAD2DEG;
        target = -target; // flip for visual

        float diff = target - vehicle->angle;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff <= -180.0f) diff += 360.0f;

        vehicle->angle += diff * ROTATE_BACK_SPEED * dt;
        if (vehicle->angle > 180.0f) vehicle->angle -= 360.0f;
        if (vehicle->angle <= -180.0f) vehicle->angle += 360.0f;

        // extra tilt from player input
        float tilt = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) {
            // lean back when accelerating
            tilt = -vehicle->velocity.x * 2.0f; // tweak factor to slow tilt based on speed
        } else if (IsKeyDown(KEY_LEFT)) {
            // lean forward when braking
            tilt = 20.0f; // instant forward tilt
        }

        // apply extra tilt on top of terrain rotation
        vehicle->angle += tilt * dt;
    }
}


int findTerrainSegment(const std::vector<Vector2>& terrain, float x) {
    if (terrain.size() < 2) return -1;
    if (x <= terrain.front().x) return 0;
    if (x >= terrain.back().x) return (int)terrain.size() - 2;
    // linear scan (ok for moderate vector sizes)
    for (size_t i = 0; i + 1 < terrain.size(); ++i) {
        if (x >= terrain[i].x && x <= terrain[i + 1].x) return (int)i;
    }
    return (int)terrain.size() - 2;
}

void vehicleMove(Vehicle* vehicle, const std::vector<Vector2>& terrain, float dt, bool debugDraw) {
    vehicle->position.x += vehicle->velocity.x;
    vehicle->position.y += vehicle->velocity.y;

    if (vehicle->back_wheel.on_ground && terrain.size() >= 2) {
        int terrain_index = findTerrainSegment(terrain, vehicle->back_wheel.position.x);
        if (terrain_index >= 0) {
            Vector2 point1 = terrain[terrain_index];
            Vector2 point2 = terrain[terrain_index + 1];

            if (debugDraw) {
                DrawCircleV(point1, 6, RED);
                DrawCircleV(point2, 6, ORANGE);
            }

            float friction = vehicle->velocity.x * FRICTION;
            vehicle->velocity.x -= friction * dt;
        }
    }

    if (vehicle->front_wheel.on_ground && terrain.size() >= 2) {
        int terrain_index = findTerrainSegment(terrain, vehicle->front_wheel.position.x);
        if (terrain_index >= 0) {
            Vector2 point1 = terrain[terrain_index];
            Vector2 point2 = terrain[terrain_index + 1];

            if (debugDraw) {
                DrawCircleV(point1, 6, RED);
                DrawCircleV(point2, 6, ORANGE);
            }

            float friction = vehicle->velocity.x * FRICTION;
            vehicle->velocity.x -= friction * dt;
        }
    }

    float max_y = GetScreenHeight() - vehicle->height / 2;

    if (vehicle->position.y >= max_y) {
        vehicle->velocity.y = 0;
        vehicle->position.y = max_y;
    } else {
        vehicle->velocity.y += GRAVITY * dt;
    }
}


void vehicleAapplySuspension(Vehicle* vehicle, Wheel* wheel, float dt) {
    Vector2 bottom_direction = Vector2Rotate({0, 1}, vehicle->angle * DEG2RAD);

    // local attachment point on vehicle body (physics)
    Vector2 attachment_point_local = {
        -vehicle->width / 2 + wheel->padding + wheel->radius + wheel->offset,
        wheel->attachOffsetY
    };
    Vector2 attachment_point = Vector2Rotate(attachment_point_local, vehicle->angle * DEG2RAD);
    attachment_point = Vector2Add(attachment_point, vehicle->position);

    // removed green circle for debugging

    float length = Vector2Distance(wheel->position, attachment_point);
    float resting_length = vehicle->height / 2 + wheel->padding + wheel->radius;
    float stretch = length - resting_length;

    // reposition wheel along bottom direction (prevents wheels drifting away visually)
    wheel->position = Vector2Add(attachment_point, Vector2Scale(bottom_direction, length));

    float spring_force = stretch * wheel->stiffness * dt;
    Vector2 relative_velocity = Vector2Subtract(vehicle->velocity, wheel->velocity);
    Vector2 damping_force = Vector2Scale(relative_velocity, wheel->damping * dt);

    Vector2 force = Vector2Subtract(Vector2Scale(bottom_direction, spring_force), damping_force);

    vehicle->velocity = Vector2Add(vehicle->velocity, force);
    wheel->velocity = Vector2Subtract(wheel->velocity, Vector2Scale(force, 0.7f));
}


void wheelMove(Wheel* wheel, const std::vector<Vector2>& terrain, float dt) {
    wheel->position.x += wheel->velocity.x;
    wheel->position.y += wheel->velocity.y;

    wheel->on_ground = false;

    if (terrain.size() >= 2) {
        for (size_t i = 1; i < terrain.size(); i++) {
            Vector2 point1 = terrain[i - 1];
            Vector2 point2 = terrain[i];

            Vector2 collision_point = {0};
            Vector2 bottom_of_wheel = {wheel->position.x, wheel->position.y + wheel->radius};
            if (IsPointBelowLine(point1, point2, bottom_of_wheel, &collision_point)) {
                wheel->velocity.y = 0;
                wheel->position.y = collision_point.y - wheel->radius + 1;
                wheel->on_ground = true;
                break;
            }
        }
    }

    if (!wheel->on_ground) {
        wheel->velocity.y += GRAVITY * dt;
    }
}

// Draws textures + debug overlays. Uses spriteOffset to nudge visuals without touching physics.
void vehicleDraw(Vehicle* vehicle, Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw = true) {
    // REAR WHEEL
    {
        Wheel& w = vehicle->back_wheel;
        float diameter = w.radius * 2.0f;
        float texAspect = (float)wheelRearTex.height / (float)wheelRearTex.width;
        Rectangle src = { 0.0f, 0.0f, (float)wheelRearTex.width, (float)wheelRearTex.height };
        Rectangle dest = {
            w.position.x - diameter * 0.5f + w.spriteOffset.x,
            w.position.y - diameter * 0.5f + w.spriteOffset.y,
            diameter * w.spriteScale, diameter * w.spriteScale * texAspect
        };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };
        DrawTexturePro(wheelRearTex, src, dest, origin, w.visualRotation, WHITE);

        if (debugDraw) DrawCircleLines((int)w.position.x, (int)w.position.y, w.radius, RED);
    }

    // FRONT WHEEL
    {
        Wheel& w = vehicle->front_wheel;
        float diameter = w.radius * 2.0f;
        float texAspect = (float)wheelFrontTex.height / (float)wheelFrontTex.width;
        Rectangle src = { 0.0f, 0.0f, (float)wheelFrontTex.width, (float)wheelFrontTex.height };
        Rectangle dest = {
            w.position.x - diameter * 0.5f + w.spriteOffset.x,
            w.position.y - diameter * 0.5f + w.spriteOffset.y,
            diameter * w.spriteScale, diameter * w.spriteScale * texAspect
        };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };
        DrawTexturePro(wheelFrontTex, src, dest, origin, w.visualRotation, WHITE);

        if (debugDraw) DrawCircleLines((int)w.position.x, (int)w.position.y, w.radius, RED);
    }

    if (debugDraw) {
    // Draw driver head above and slightly to the left of vehicle center
        Vector2 headPos = {
            vehicle->position.x - 42.5f + vehicle->spriteOffset.x,
            vehicle->position.y - vehicle->height * 0.65f + vehicle->spriteOffset.y
        };
        float headRadius = 15;

    // Draw transparent circle fill
        DrawCircleV(headPos, headRadius, (Color){0, 0, 255, 50}); // semi-transparent blue

        // Draw circle outline
        DrawCircleLines((int)headPos.x, (int)headPos.y, headRadius, BLUE);

        // Draw text inside
        const char* label = "HEAD";
        int fontSize = 14;
        Vector2 textSize = MeasureTextEx(GetFontDefault(), label, (float)fontSize, 1);
        DrawText(label, headPos.x - textSize.x * 0.5f, headPos.y - textSize.y * 0.5f, fontSize, BLACK);
    }
    
    // BODY TEXTURE (draw last)
    float bodyW = vehicle->width;
    float bodyH = vehicle->height;
    Rectangle srcBody = { 0.0f, 0.0f, (float)bodyTex.width, (float)bodyTex.height };
    Rectangle destBody = {
        vehicle->position.x - bodyW * 0.5f + vehicle->spriteOffset.x,
        vehicle->position.y - bodyH * 0.5f + vehicle->spriteOffset.y,
        bodyW, bodyH
    };
    Vector2 originBody = { bodyW * 0.5f - vehicle->spriteOffset.x, bodyH * 0.5f - vehicle->spriteOffset.y };
    DrawTexturePro(bodyTex, srcBody, destBody, originBody, vehicle->angle, WHITE);

    // removed body debug fill and outline
}


int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Uphill Racer - Sprite Offsets");
    SetTargetFPS(144);

    // Load textures (place your PNGs in assets/)
    Texture2D bodyTexture = LoadTexture("assets/body.png");
    Texture2D wheelFrontTexture = LoadTexture("assets/wheel_front.png");
    Texture2D wheelRearTexture = LoadTexture("assets/wheel_rear.png");

    Camera2D camera = {};
    camera.offset = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    camera.target = {0, 0};
    camera.rotation = 0;
    camera.zoom = 1;

    Vehicle vehicle = {};
    vehicle.position = {1200, 300};
    vehicle.width = 250;
    vehicle.height = 100;
    vehicle.angle = 0;
    vehicle.spriteOffset = {75.5, 52.5};
    
    float initialX = vehicle.position.x;

    float stifneess = 1.0f;
    float damping = 2.0f;
    float padding = 10.0f;

    vehicle.back_wheel.radius = 32.5f;
    vehicle.back_wheel.padding = padding;
    vehicle.back_wheel.stiffness = stifneess;
    vehicle.back_wheel.damping = damping;
    vehicle.back_wheel.offset = 20.0f;
    vehicle.back_wheel.attachOffsetY = 0.0f;
    vehicle.back_wheel.spriteOffset = {32.5, 35};
    vehicle.back_wheel.spriteScale = 1.1f;
    vehicle.back_wheel.position = {vehicle.position.x - vehicle.width / 2 + vehicle.back_wheel.radius + vehicle.back_wheel.padding,
                               vehicle.position.y + vehicle.height / 2 + vehicle.back_wheel.radius + vehicle.back_wheel.padding};

    // Front wheel physics & visuals
    vehicle.front_wheel.radius = 32.5f;
    vehicle.front_wheel.padding = padding;
    vehicle.front_wheel.stiffness = stifneess;
    vehicle.front_wheel.damping = damping;
    vehicle.front_wheel.offset = 175.0f;
    vehicle.front_wheel.attachOffsetY = 0.0f;
    vehicle.front_wheel.spriteOffset = {32.5, 35};
    vehicle.front_wheel.spriteScale = 1.1f;
    vehicle.front_wheel.position = {vehicle.position.x + vehicle.width / 2 - vehicle.front_wheel.radius - vehicle.front_wheel.padding,
                                vehicle.position.y + vehicle.height / 2 + vehicle.front_wheel.radius + vehicle.front_wheel.padding};


    // Terrain generation params
    const float SEGMENT_MEAN = 220.0f; // bigger spacing between points
    const int SEGMENT_RANDOM_OFFSET = 60;
    const int HEIGHT_RANDOM = 80;
    const float JUMP_PROBABILITY = 0.15f;

    const float REMOVAL_PADDING = (float)WINDOW_WIDTH * 2.0f;
    const float GENERATE_AHEAD = (float)WINDOW_WIDTH * 2.0f;

    std::vector<Vector2> terrain;
    terrain.reserve(512);

    float startX = -SEGMENT_MEAN * 4.0f;
    int posY = GetRandomValue((int)(WINDOW_HEIGHT * 0.6f), (int)(WINDOW_HEIGHT * 0.85f));
    float curX = startX;
    int initialSegments = 30;

    for (int i = 0; i < initialSegments; ++i) {
        float nextX = curX + SEGMENT_MEAN + GetRandomValue(-SEGMENT_RANDOM_OFFSET, SEGMENT_RANDOM_OFFSET);

        // Decide next Y
        int moveY;
        if (((float)GetRandomValue(0, 100) / 100.0f) < JUMP_PROBABILITY) {
            // create a jump: steep drop
            moveY = GetRandomValue(HEIGHT_RANDOM, HEIGHT_RANDOM * 2) * -1; // negative for downward jump
        } else {
            moveY = GetRandomValue(-HEIGHT_RANDOM, HEIGHT_RANDOM); // normal terrain
        }

        int nextY = (int)clampf((float)posY + moveY, WINDOW_HEIGHT * 0.25f, WINDOW_HEIGHT * 0.95f);

        terrain.push_back({nextX, (float)nextY});
        posY = nextY;
        curX = nextX;
    }

    bool debugDraw = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // toggle debug draw
        if (IsKeyPressed(KEY_D)) debugDraw = !debugDraw;

        BeginDrawing();
        BeginMode2D(camera);

        camera.target = vehicle.position;
        camera.zoom = 0.8f - vehicle.velocity.x / 10.0f;
        if (camera.zoom > 1.3f) camera.zoom = 1.3f;
        if (camera.zoom < 0.8f) camera.zoom = 0.8f;

        ClearBackground(WHITE);

        // generate terrain ahead if needed
        while (terrain.empty() || terrain.back().x < camera.target.x + GENERATE_AHEAD) {
            Vector2 last = terrain.empty() ? Vector2{curX, (float)posY} : terrain.back();
            int moveY = GetRandomValue(-HEIGHT_RANDOM, HEIGHT_RANDOM);
            float nextX = last.x + SEGMENT_MEAN + GetRandomValue(-SEGMENT_RANDOM_OFFSET, SEGMENT_RANDOM_OFFSET);
            float nextY = clampf(last.y + moveY, WINDOW_HEIGHT * 0.25f, WINDOW_HEIGHT * 0.95f);
            terrain.push_back({nextX, nextY});
        }
        // trim far behind
        while (terrain.size() > 6 && terrain.front().x < camera.target.x - REMOVAL_PADDING) {
            terrain.erase(terrain.begin());
        }

        // draw terrain
        for (size_t i = 1; i < terrain.size(); ++i) {
            DrawLineEx(terrain[i - 1], terrain[i], 5, BLACK);
        }

        // update physics
        vehicleControl(&vehicle, dt);
        vehicleMove(&vehicle, terrain, dt, debugDraw);
        vehicleRotate(&vehicle, dt);
        wheelMove(&vehicle.back_wheel, terrain, dt);
        wheelMove(&vehicle.front_wheel, terrain, dt);
        vehicleAapplySuspension(&vehicle, &vehicle.back_wheel, dt);
        vehicleAapplySuspension(&vehicle, &vehicle.front_wheel, dt);

        // draw vehicle sprites with debug overlays (spriteOffset used here)
        vehicleDraw(&vehicle, bodyTexture, wheelFrontTexture, wheelRearTexture, debugDraw);

        EndMode2D();

        // --- HUD ---
        float distanceMeters = (vehicle.position.x - initialX) / 75.0f;
        float speedScaled = vehicle.velocity.x * 5.0f;

        DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, BLACK);
        DrawText(TextFormat("Distance: %.0f m", distanceMeters), 10, 40, 20, BLACK);
        DrawText(TextFormat("Speed: %.0f", speedScaled), 10, 70, 20, BLACK);

        EndDrawing();
    }

    UnloadTexture(wheelFrontTexture);
    UnloadTexture(wheelRearTexture);
    UnloadTexture(bodyTexture);

    CloseWindow();
    return 0;
}