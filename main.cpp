#include "raylib.h"
#include "Player.h"
#include "CameraController.h"

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Gravity Defied Prototype (C++/Raylib)");
    SetTargetFPS(144);

    Player player({400, 280});
    CameraController camera(player.position, screenWidth, screenHeight);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Update
        player.Update(deltaTime);
        camera.Update(player, deltaTime);

        if (IsKeyPressed(KEY_R)) player.Reset({400, 280});

        // Draw
        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        BeginMode2D(camera.camera);

        // Draw ground 
        int thicknees = 10;
        int groundY = 400;
        DrawRectangle(0, groundY - thicknees/2, screenWidth * 10, thicknees, DARKGREEN);

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
