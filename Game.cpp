#include "Game.h"
#include <raymath.h>
#include <raylib.h>
#include "Config.h"
#include "Utils.h"

Game::Game() : debugDraw(true) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Uphill Racer - Refactored");
    SetTargetFPS(144);

    // Load assets
    bodyTex = LoadTexture("assets/body.png");
    wheelFrontTex = LoadTexture("assets/wheel_front.png");
    wheelRearTex = LoadTexture("assets/wheel_rear.png");

    // Setup camera
    camera.offset = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    camera.rotation = 0;
    camera.zoom = 1.0f;

    // Set player start X position
    float playerStartX = 1200.0f;

    // Generate terrain with plenty behind and ahead of the player
    terrain.GenerateInitial(
        playerStartX - 2000.0f,       // 2000 px behind
        WINDOW_WIDTH * 3.0f,          // 3 screens wide ahead
        220.0f,                       // segment mean
        60.0f,                        // segment random offset
        80.0f,                        // height random
        0.15f,                        // jump probability
        WINDOW_HEIGHT * 0.25f,        // min Y
        WINDOW_HEIGHT * 0.95f         // max Y
    );

    // Initialize vehicle
    vehicle.width = 250;
    vehicle.height = 100;
    vehicle.angle = 0;
    vehicle.spriteOffset = {75.5f, 52.5f};

    vehicle.position.x = playerStartX;

    // Get ground height at playerStartX
    float groundY = terrain.GetHeightAt(playerStartX);
    vehicle.position.y = groundY - 150.0f; // Offset so player floats above ground

    // Store initial X for distance calculation
    initialX = vehicle.position.x;

    // Camera target on player
    camera.target = vehicle.position;
}

Game::~Game() {
    UnloadTexture(bodyTex);
    UnloadTexture(wheelFrontTex);
    UnloadTexture(wheelRearTex);
    CloseWindow();
}

void Game::Run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        HandleInput();
        Update(dt);
        Draw();
    }
}

void Game::HandleInput() {
    if (IsKeyPressed(KEY_D)) debugDraw = !debugDraw;
}

void Game::Update(float dt) {
    vehicle.Control(dt);
    vehicle.Rotate(dt);
    vehicle.Move(terrain.points, dt, debugDraw);

    // Update camera to follow player
    camera.target = vehicle.position;
    camera.zoom = 0.8f - vehicle.velocity.x / 10.0f;
    camera.zoom = clampf(camera.zoom, 0.8f, 1.3f);

    // Extend terrain ahead and trim behind
    terrain.GenerateAhead(camera.target.x, WINDOW_WIDTH * 2.0f, 220, 60, 80, 0.15f);
    terrain.TrimBehind(camera.target.x, WINDOW_WIDTH * 2.0f);
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(WHITE);

    BeginMode2D(camera);

    terrain.Draw();
    vehicle.Draw(bodyTex, wheelFrontTex, wheelRearTex, debugDraw);

    EndMode2D();

    float distanceMeters = (vehicle.position.x - initialX) / 75.0f;
    float speedScaled = vehicle.velocity.x * 5.0f;

    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, BLACK);
    DrawText(TextFormat("Distance: %.0f m", distanceMeters), 10, 40, 20, BLACK);
    DrawText(TextFormat("Speed: %.0f", speedScaled), 10, 70, 20, BLACK);

    EndDrawing();
}