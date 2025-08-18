#pragma once
#include "raylib.h"

class Player 
{
    public:
        Vector2 position;
        Vector2 velocity;
        bool canJump;
        float startX;

    Player(Vector2 startPos = {400, 280});
    void Update(float delta);
    void Reset(Vector2 startPos);
    void Draw() const;
    float GetDistance() const;
};
