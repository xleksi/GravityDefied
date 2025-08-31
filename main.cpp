// main.cpp
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>

#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 1000

#define GRAVITY 7
#define FRICTION 0.2f
#define ROTATION_SPEED 30
#define ROTATE_BACK_SPEED 3
#define CAR_SPEED 3
#define HILL_SPEED 0.2f
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

struct Car {
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

void car_control(Car* car, float dt) {
    if (!car->back_wheel.on_ground && !car->front_wheel.on_ground) {
        if (IsKeyDown(KEY_LEFT)) {
            car->angle += ROTATION_SPEED * dt;
        } else if (IsKeyDown(KEY_RIGHT)) {
            car->angle -= ROTATION_SPEED * dt;
        }
    }

    if (IsKeyDown(KEY_RIGHT)) {
        if (car->back_wheel.on_ground) car->velocity.x += CAR_SPEED * dt;
        if (car->front_wheel.on_ground) car->velocity.x += CAR_SPEED * dt;
    } else if (IsKeyDown(KEY_LEFT)) {
        if (car->back_wheel.on_ground) car->velocity.x -= CAR_SPEED * dt;
        if (car->front_wheel.on_ground) car->velocity.x -= CAR_SPEED * dt;
    }
}

void car_rotate(Car* car, float dt) {
    if (car->back_wheel.on_ground && car->front_wheel.on_ground) {
        float target = Vector2LineAngle(car->back_wheel.position, car->front_wheel.position) * RAD2DEG;
        target = -target; // visual flip

        float diff = target - car->angle;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff <= -180.0f) diff += 360.0f;

        car->angle += diff * ROTATE_BACK_SPEED * dt;

        if (car->angle > 180.0f) car->angle -= 360.0f;
        if (car->angle <= -180.0f) car->angle += 360.0f;
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

void car_move(Car* car, const std::vector<Vector2>& terrain, float dt) {
    car->position.x += car->velocity.x;
    car->position.y += car->velocity.y;

    if (car->back_wheel.on_ground && terrain.size() >= 2) {
        int terrain_index = findTerrainSegment(terrain, car->back_wheel.position.x);
        if (terrain_index >= 0) {
            Vector2 point1 = terrain[terrain_index];
            Vector2 point2 = terrain[terrain_index + 1];
            DrawCircleV(point1, 6, RED);
            DrawCircleV(point2, 6, ORANGE);

            float friction = car->velocity.x * FRICTION;
            car->velocity.x -= friction * dt;
        }
    }

    if (car->front_wheel.on_ground && terrain.size() >= 2) {
        int terrain_index = findTerrainSegment(terrain, car->front_wheel.position.x);
        if (terrain_index >= 0) {
            Vector2 point1 = terrain[terrain_index];
            Vector2 point2 = terrain[terrain_index + 1];
            DrawCircleV(point1, 6, RED);
            DrawCircleV(point2, 6, ORANGE);

            float friction = car->velocity.x * FRICTION;
            car->velocity.x -= friction * dt;
        }
    }

    float max_y = GetScreenHeight() - car->height / 2;

    if (car->position.y >= max_y) {
        car->velocity.y = 0;
        car->position.y = max_y;
    } else {
        car->velocity.y += GRAVITY * dt;
    }
}

void car_apply_suspension(Car* car, Wheel* wheel, float dt) {
    Vector2 bottom_direction = Vector2Rotate({0, 1}, car->angle * DEG2RAD);

    // local attachment point on car body (physics)
    Vector2 attachment_point_local = {
        -car->width / 2 + wheel->padding + wheel->radius + wheel->offset,
        wheel->attachOffsetY
    };
    Vector2 attachment_point = Vector2Rotate(attachment_point_local, car->angle * DEG2RAD);
    attachment_point = Vector2Add(attachment_point, car->position);

    // draw attachment for debug
    DrawCircleV(attachment_point, 5, GREEN);

    float length = Vector2Distance(wheel->position, attachment_point);
    float resting_length = car->height / 2 + wheel->padding + wheel->radius;
    float stretch = length - resting_length;

    // reposition wheel along bottom direction (prevents wheels drifting away visually)
    wheel->position = Vector2Add(attachment_point, Vector2Scale(bottom_direction, length));

    float spring_force = stretch * wheel->stiffness * dt;
    Vector2 relative_velocity = Vector2Subtract(car->velocity, wheel->velocity);
    Vector2 damping_force = Vector2Scale(relative_velocity, wheel->damping * dt);

    Vector2 force = Vector2Subtract(Vector2Scale(bottom_direction, spring_force), damping_force);

    car->velocity = Vector2Add(car->velocity, force);
    wheel->velocity = Vector2Subtract(wheel->velocity, Vector2Scale(force, 0.7f));
}

void wheel_move(Wheel* wheel, const std::vector<Vector2>& terrain, float dt) {
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
void car_draw(Car* car, Texture2D bodyTex, Texture2D wheelFrontTex, Texture2D wheelRearTex, bool debugDraw = true) {
    // REAR WHEEL (draw first)
    {
        Wheel& w = car->back_wheel;
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

    // FRONT WHEEL (draw second)
    {
        Wheel& w = car->front_wheel;
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

    // BODY DEBUG FILL (draw before body texture so it shows through transparent PNG areas)
    float bodyW = car->width;
    float bodyH = car->height;
    Rectangle destBodyDebug = {
        car->position.x - bodyW * 0.5f + car->spriteOffset.x,
        car->position.y - bodyH * 0.5f + car->spriteOffset.y,
        bodyW, bodyH
    };
    Vector2 originBody = { bodyW * 0.5f - car->spriteOffset.x, bodyH * 0.5f - car->spriteOffset.y };

    if (debugDraw) {
        // semi-transparent fill (will be visible through body texture alpha)
        DrawRectanglePro(destBodyDebug, originBody, car->angle, (Color){0, 255, 0, 40});
    }

    // BODY TEXTURE (draw last so it appears over wheels)
    Rectangle srcBody = { 0.0f, 0.0f, (float)bodyTex.width, (float)bodyTex.height };
    Rectangle destBody = {
        car->position.x - bodyW * 0.5f + car->spriteOffset.x,
        car->position.y - bodyH * 0.5f + car->spriteOffset.y,
        bodyW, bodyH
    };
    DrawTexturePro(bodyTex, srcBody, destBody, originBody, car->angle, WHITE);

    // BODY OUTLINE (draw after body texture so outline is visible)
    if (debugDraw) {
        drawRotatedRectOutline(car->position + car->spriteOffset, bodyW, bodyH, car->angle, 3.0f, (Color){0, 200, 0, 255});
    }
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Uphill Racer - Sprite Offsets");
    SetTargetFPS(60);

    // Load textures (place your PNGs in assets/)
    Texture2D bodyTexture = LoadTexture("assets/body.png");
    Texture2D wheelFrontTexture = LoadTexture("assets/wheel_front.png");
    Texture2D wheelRearTexture = LoadTexture("assets/wheel_rear.png");

    Camera2D camera = {};
    camera.offset = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    camera.target = {0, 0};
    camera.rotation = 0;
    camera.zoom = 1;

    Car car = {};
    car.position = {1200, 300};
    car.width = 250;
    car.height = 100;
    car.angle = 0;
    car.spriteOffset = {75.5, 52.5}; // tweak this to move body sprite

    // Back wheel physics & visuals
    car.back_wheel.radius = 32.5f;
    car.back_wheel.padding = 10;
    car.back_wheel.stiffness = 0.8f;
    car.back_wheel.damping = 0.6f;
    car.back_wheel.offset = 20.0f; // horizontal physics attach tweak
    car.back_wheel.attachOffsetY = 0.0f; // vertical physics attach tweak
    car.back_wheel.spriteOffset = {32.5, 35}; // visual sprite nudge
    car.back_wheel.spriteScale = 1.1f;
    car.back_wheel.position = {car.position.x - car.width / 2 + car.back_wheel.radius + car.back_wheel.padding,
                               car.position.y + car.height / 2 + car.back_wheel.radius + car.back_wheel.padding};

    // Front wheel physics & visuals
    car.front_wheel.radius = 32.5f;
    car.front_wheel.padding = 10;
    car.front_wheel.stiffness = 0.8f;
    car.front_wheel.damping = 0.6f;
    car.front_wheel.offset = 180.0f;
    car.front_wheel.attachOffsetY = 0.0f;
    car.front_wheel.spriteOffset = {32.5, 35};
    car.front_wheel.spriteScale = 1.1f;
    car.front_wheel.position = {car.position.x + car.width / 2 - car.front_wheel.radius - car.front_wheel.padding,
                                car.position.y + car.height / 2 + car.front_wheel.radius + car.front_wheel.padding};

    // Terrain generation params
    const float SEGMENT_MEAN = 220.0f; // bigger spacing between points
    const int SEGMENT_RANDOM_OFFSET = 50;
    const int HEIGHT_RANDOM = 30;
    const float REMOVAL_PADDING = (float)WINDOW_WIDTH * 2.0f;
    const float GENERATE_AHEAD = (float)WINDOW_WIDTH * 2.0f;

    std::vector<Vector2> terrain;
    terrain.reserve(512);

    float startX = -SEGMENT_MEAN * 4.0f;
    int posY = GetRandomValue((int)(WINDOW_HEIGHT * 0.6f), (int)(WINDOW_HEIGHT * 0.85f));
    int initialSegments = 30;
    float curX = startX;
    for (int i = 0; i < initialSegments; ++i) {
        int moveY = GetRandomValue(-HEIGHT_RANDOM, HEIGHT_RANDOM);
        terrain.push_back({curX, (float)posY});
        posY = (int)clampf((float)posY + moveY, WINDOW_HEIGHT * 0.25f, WINDOW_HEIGHT * 0.95f);
        curX += SEGMENT_MEAN + GetRandomValue(-SEGMENT_RANDOM_OFFSET, SEGMENT_RANDOM_OFFSET);
    }

    bool debugDraw = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // toggle debug draw
        if (IsKeyPressed(KEY_D)) debugDraw = !debugDraw;

        BeginDrawing();
        BeginMode2D(camera);

        camera.target = car.position;
        camera.zoom = 1.3f - car.velocity.x / 10.0f;
        if (camera.zoom > 1.3f) camera.zoom = 1.3f;
        if (camera.zoom < 1.0f) camera.zoom = 1.0f;

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
        car_control(&car, dt);
        car_move(&car, terrain, dt);
        car_rotate(&car, dt);
        wheel_move(&car.back_wheel, terrain, dt);
        wheel_move(&car.front_wheel, terrain, dt);
        car_apply_suspension(&car, &car.back_wheel, dt);
        car_apply_suspension(&car, &car.front_wheel, dt);

        // draw car sprites with debug overlays (spriteOffset used here)
        car_draw(&car, bodyTexture, wheelFrontTexture, wheelRearTexture, debugDraw);

        EndMode2D();
        EndDrawing();
    }

    UnloadTexture(wheelFrontTexture);
    UnloadTexture(wheelRearTexture);
    UnloadTexture(bodyTexture);

    CloseWindow();
    return 0;
}