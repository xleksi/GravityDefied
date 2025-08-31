#pragma once
#include "Vehicle.h"
#include "Terrain.h"
#include <raylib.h>

class Game {
private:
    Vehicle vehicle;
    Terrain terrain;
    Camera2D camera;
    float initialX;
    bool debugDraw;

    Texture2D bodyTex;
    Texture2D wheelFrontTex;
    Texture2D wheelRearTex;

public:
    Game();
    ~Game();

    void Run();

private:
    void HandleInput();
    void Update(float dt);
    void Draw();
};
