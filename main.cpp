#include <iostream>
#include <raylib.h>

int main(int, char**){
    InitWindow(800, 600, "GravityDefied");
    SetTargetFPS(144);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Hello, GravityDefied!", 190, 200, 20, DARKGRAY);

        DrawFPS(10, 10); // Draw FPS counter
        EndDrawing();
    }

    CloseWindow(); // Close window and OpenGL context
    std::cout << "Hello, from GravityDefied!\n";
    return 0; // Exit the program
}
