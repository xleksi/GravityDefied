#include "raylib.h"
#include "Player.h"
#include "CameraController.h"
#include "Collision.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() 
{
    std::cout << "CWD: " << fs::current_path() << '\n';
    if (fs::exists("assets"))
        for (auto& p : fs::directory_iterator("assets"))
            std::cout << p.path().filename() << '\n';

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Gravity Defied Prototype (C++/Raylib)");
    SetTargetFPS(144);

    Player player({400, 280});
    CameraController camera(player.position, screenWidth, screenHeight);

    const int groundY = 400;
    const int thickness = 10;
    Rectangle ground = {0, groundY - thickness/2.0f, screenWidth * 10.0f, (float)thickness};

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Update
        player.Update(deltaTime, ground);
        camera.Update(player, deltaTime);

        if (IsKeyPressed(KEY_R)) player.Reset({400, 280});

        // Draw
        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        BeginMode2D(camera.camera);

        DrawRectangleRec(ground, DARKGREEN);

        player.Draw();

        EndMode2D();

        DrawText("Controls: A/D to move, SPACE to jump, R to reset", 20, 20, 10, DARKGRAY);
        float distance = player.GetDistance()/100.0f; // Convert to meters
        DrawText(TextFormat("Distance: %.1f", distance), 20, 50, 20, BLACK);
        DrawFPS(screenWidth - 100, 10);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
