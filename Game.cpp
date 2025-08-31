#include "Game.h"
#include "Config.h"
#include "Utils.h"
#include <raylib.h>
#include <raymath.h>

// Game constructor: generate terrain first, then place vehicle high so it drops
Game::Game() : debugDraw(true) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Uphill Racer - Structured");
    SetTargetFPS(144);

    bodyTex = LoadTexture("assets/body.png");
    wheelFrontTex = LoadTexture("assets/wheel_front.png");
    wheelRearTex = LoadTexture("assets/wheel_rear.png");

    // Came
    camera.offset = { (float)WINDOW_WIDTH * 0.5f, (float)WINDOW_HEIGHT * 0.5f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    // player start X
    float playerStartX = 1200.0f;

    // initial terrain behind and infront
    terrain.GenerateInitial(
        playerStartX - 2000.0f,      // start far behind
        WINDOW_WIDTH * 3.0f,         // generate ahead
        220.0f,                      // segment mean
        60,                          // random offset
        80,                          // height random
        0.15f,                       // jump probability
        WINDOW_HEIGHT * 0.25f,       // min Y
        WINDOW_HEIGHT * 0.95f        // max Y
    );

    vehicle.width = 250.0f;
    vehicle.height = 100.0f;
    vehicle.spriteOffset = {75.5f, 52.5f};


    float spawnY = WINDOW_HEIGHT * -0.35f; // spawn hight for drop
    vehicle.PlaceAt(playerStartX, spawnY);

    initialX = vehicle.position.x;

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
    if (IsKeyPressed(KEY_Z)) debugDraw = !debugDraw;
}

void Game::Update(float dt) {
    vehicle.Control(dt);

    vehicle.MoveBody(dt);

    vehicle.Rotate(dt);

    vehicle.backWheel.Move(terrain.points, dt);
    vehicle.frontWheel.Move(terrain.points, dt);

    vehicle.backWheel.UpdateRotation();
    vehicle.frontWheel.UpdateRotation();

    vehicle.backWheel.ApplySuspension(vehicle.position, vehicle.angle, vehicle.velocity, vehicle.height, vehicle.width, dt);
    vehicle.frontWheel.ApplySuspension(vehicle.position, vehicle.angle, vehicle.velocity, vehicle.height, vehicle.width, dt);

    camera.target = vehicle.position;
    camera.zoom = 0.8f - vehicle.velocity.x / 10.0f;
    camera.zoom = clampf(camera.zoom, 0.8f, 1.3f);

    terrain.GenerateAhead(camera.target.x, WINDOW_WIDTH * 2.0f, 220.0f, 60, 80, 0.15f);
    terrain.TrimBehind(camera.target.x, WINDOW_WIDTH * 2.0f);
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(LIGHTGRAY);

    BeginMode2D(camera);

    terrain.Draw();
    vehicle.Draw(bodyTex, wheelFrontTex, wheelRearTex, debugDraw);

    EndMode2D();

    // HUD
    float distanceMeters = (vehicle.position.x - initialX) / 75.0f;
    float speedScaled = vehicle.velocity.x * 5.0f;
    DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, BLACK);
    DrawText(TextFormat("Distance: %.0f m", distanceMeters), 10, 40, 20, BLACK);
    DrawText(TextFormat("Speed: %.0f", speedScaled), 10, 70, 20, BLACK);
    DrawText("Use A/D or Left/Right to drive, Z to toggle debug", 10, 100, 20, BLACK);

    EndDrawing();
}
