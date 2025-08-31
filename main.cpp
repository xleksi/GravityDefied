#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstdio>

#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 1000

#define GRAVITY 7
#define FRICTION 0.2f
#define ROTATION_SPEED 30
#define ROTATE_BACK_SPEED 3
#define CAR_SPEED 3
#define HILL_SPEED 0.2f
#define TRANSPARENT_BLACK (Color){0, 0, 0, 100}

bool IsPointBelowLine(Vector2 a, Vector2 b, Vector2 point, Vector2* collisionPoint) {
    if (fabs(b.x - a.x) > 1e-6f) {
        float t = (point.x - a.x) / (b.x - a.x);
        if (t < 0.0f || t > 1.0f) return false;

        collisionPoint->x = point.x;
        collisionPoint->y = a.y + t * (b.y - a.y);
        return (point.y > collisionPoint->y);
    } else {
        if (fabs(point.x - a.x) > 1e-6f) return false;

        collisionPoint->x = a.x;
        collisionPoint->y = fmax(a.y, b.y);
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
    float offset;
    bool on_ground;
};

struct Car {
    Vector2 position;
    Vector2 velocity;
    float width;
    float height;
    float angle;
    Wheel back_wheel;
    Wheel front_wheel;
};

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

// ✅ Fixed rotation (align to slope correctly)
void car_rotate(Car* car, float dt) {
    if (car->back_wheel.on_ground && car->front_wheel.on_ground) {
        float target = Vector2LineAngle(car->back_wheel.position, car->front_wheel.position) * RAD2DEG;
        target = -target; // Flip sign for correct visual alignment

        float diff = target - car->angle;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff <= -180.0f) diff += 360.0f;

        car->angle += diff * ROTATE_BACK_SPEED * dt;

        if (car->angle > 180.0f) car->angle -= 360.0f;
        if (car->angle <= -180.0f) car->angle += 360.0f;

        printf("Rotate: target=%f diff=%f angle=%f\n", target, diff, car->angle);
    }
}

// ✅ Removed auto-acceleration on slopes
void car_move(Car* car, Vector2 terrain[], int terrain_length, float dt) {
    car->position.x += car->velocity.x;
    car->position.y += car->velocity.y;

    if (car->back_wheel.on_ground) {
        int terrain_index = (int)floor(car->back_wheel.position.x / terrain_length);
        Vector2 point1 = terrain[terrain_index];
        Vector2 point2 = terrain[terrain_index + 1];

        DrawCircleV(point1, 10, RED);
        DrawCircleV(point2, 10, ORANGE);

        float angle = Vector2LineAngle(point1, point2) * RAD2DEG;
        printf("Terrain angle (back wheel): %f (%d)\n", angle, terrain_index);

        // REMOVED: car->velocity.x += angle * HILL_SPEED * dt;

        float friction = car->velocity.x * FRICTION;
        car->velocity.x -= friction * dt;
    }

    if (car->front_wheel.on_ground) {
        int terrain_index = (int)floor(car->front_wheel.position.x / terrain_length);
        Vector2 point1 = terrain[terrain_index];
        Vector2 point2 = terrain[terrain_index + 1];

        DrawCircleV(point1, 10, RED);
        DrawCircleV(point2, 10, ORANGE);

        float angle = Vector2LineAngle(point1, point2) * RAD2DEG;
        printf("Terrain angle (front wheel): %f (%d)\n", angle, terrain_index);

        // REMOVED: car->velocity.x += angle * HILL_SPEED * dt;

        float friction = car->velocity.x * FRICTION;
        car->velocity.x -= friction * dt;
    }

    float max_y = GetScreenHeight() - car->height / 2;

    if (car->position.y >= max_y) {
        car->velocity.y = 0;
        car->position.y = max_y;
    } else {
        car->velocity.y += GRAVITY * dt;
    }
}

void car_draw(Car* car) {
    DrawRectanglePro({car->position.x, car->position.y, car->width, car->height},
                     {car->width / 2, car->height / 2}, car->angle, TRANSPARENT_BLACK);

    DrawCircle(car->back_wheel.position.x, car->back_wheel.position.y, car->back_wheel.radius, TRANSPARENT_BLACK);
    DrawCircle(car->front_wheel.position.x, car->front_wheel.position.y, car->front_wheel.radius, TRANSPARENT_BLACK);
}

void car_apply_suspension(Car* car, Wheel* wheel, float dt) {
    Vector2 bottom_direction = Vector2Rotate({0, 1}, car->angle * DEG2RAD);

    Vector2 attachment_point = Vector2Rotate(
        {-car->width / 2 + wheel->padding + wheel->radius + wheel->offset, 0}, car->angle * DEG2RAD);
    attachment_point = Vector2Add(attachment_point, car->position);
    DrawCircleV(attachment_point, 10, GREEN);

    float length = Vector2Distance(wheel->position, attachment_point);
    float resting_length = car->height / 2 + wheel->padding + wheel->radius;
    float stretch = length - resting_length;

    wheel->position = Vector2Add(attachment_point, Vector2Scale(bottom_direction, length));

    float spring_force = stretch * wheel->stiffness * dt;
    Vector2 relative_velocity = Vector2Subtract(car->velocity, wheel->velocity);
    Vector2 damping_force = Vector2Scale(relative_velocity, wheel->damping * dt);

    Vector2 force = Vector2Subtract(Vector2Scale(bottom_direction, spring_force), damping_force);

    car->velocity = Vector2Add(car->velocity, force);
    wheel->velocity = Vector2Subtract(wheel->velocity, Vector2Scale(force, 0.7f));
}

void wheel_move(Wheel* wheel, Vector2 terrain[], int terrain_count, float dt) {
    wheel->position.x += wheel->velocity.x;
    wheel->position.y += wheel->velocity.y;

    wheel->on_ground = false;

    for (int i = 1; i < terrain_count; i++) {
        Vector2 point1 = terrain[i - 1];
        Vector2 point2 = terrain[i];

        Vector2 collision_point = {0};
        Vector2 bottom_of_wheel = {wheel->position.x, wheel->position.y + wheel->radius};
        if (IsPointBelowLine(point1, point2, bottom_of_wheel, &collision_point)) {
            wheel->velocity.y = 0;
            wheel->position.y = collision_point.y - wheel->radius + 1;
            wheel->on_ground = true;
        }
    }

    if (!wheel->on_ground) {
        wheel->velocity.y += GRAVITY * dt;
    }
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Uphill Racer");
    SetTargetFPS(60);

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

    car.back_wheel.radius = 25;
    car.back_wheel.padding = 10;
    car.back_wheel.stiffness = 0.8f;
    car.back_wheel.damping = 0.6f;
    car.back_wheel.position = {car.position.x - car.width / 2 + car.back_wheel.radius + car.back_wheel.padding,
                                car.position.y + car.height / 2 + car.back_wheel.radius + car.back_wheel.padding};

    car.front_wheel.radius = 25;
    car.front_wheel.padding = 10;
    car.front_wheel.stiffness = 0.8f;
    car.front_wheel.damping = 0.6f;
    car.front_wheel.offset =
        car.width - car.back_wheel.radius - car.back_wheel.padding - car.front_wheel.padding - car.front_wheel.radius;
    car.front_wheel.position = {car.position.x + car.width / 2 - car.front_wheel.radius - car.front_wheel.padding,
                                 car.position.y + car.height / 2 + car.front_wheel.radius + car.front_wheel.padding};

    int terrain_length = 100;
    int terrain_count = 255;
    Vector2* terrain = new Vector2[terrain_count];

    int pos = GetRandomValue((int)(WINDOW_HEIGHT * 0.7f), (int)(WINDOW_HEIGHT * 0.95f));
    for (int i = 0; i < terrain_count; i++) {
        int movement = GetRandomValue(-20, 20);
        terrain[i] = {(float)(i * terrain_length), (float)pos};
        pos += movement;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        BeginMode2D(camera);

        camera.target = car.position;
        camera.zoom = 1.3f - car.velocity.x / 10;
        if (camera.zoom > 1.3f) camera.zoom = 1.3f;
        if (camera.zoom < 1.0f) camera.zoom = 1.0f;

        float dt = GetFrameTime();

        ClearBackground(WHITE);

        for (int i = 1; i < terrain_count; i++) {
            DrawLineEx(terrain[i - 1], terrain[i], 5, BLACK);
        }

        car_control(&car, dt);
        car_move(&car, terrain, terrain_length, dt);
        car_rotate(&car, dt);
        wheel_move(&car.back_wheel, terrain, terrain_count, dt);
        wheel_move(&car.front_wheel, terrain, terrain_count, dt);
        car_apply_suspension(&car, &car.back_wheel, dt);
        car_apply_suspension(&car, &car.front_wheel, dt);

        car_draw(&car);

        EndMode2D();
        EndDrawing();
    }

    delete[] terrain;
    CloseWindow();
    return 0;
}
