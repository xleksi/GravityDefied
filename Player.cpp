#include "Player.h"

constexpr float GRAVITY = 800.0f;
constexpr float PLAYER_HOR_SPD = 200.0f;
constexpr float PLAYER_JUMP_SPD = 350.0f;
constexpr int GROUND_Y = 400; // y-coordinate of ground

Player::Player(Vector2 startPos) 
{
    position = startPos;
    velocity = {0.0f, 0.0f};
    canJump = false;
    startX = startPos.x;

    bodyTexture = LoadTexture("assets/body.png");
    wheelFrontTexture = LoadTexture("assets/wheel_front.png");
    wheelRearTexture = LoadTexture("assets/wheel_rear.png");

    if (bodyTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load body.png");
    if (wheelFrontTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load wheel_front.png");
    if (wheelRearTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load wheel_rear.png");

}

Player::~Player() 
{
    UnloadTexture(bodyTexture);
    UnloadTexture(wheelFrontTexture);
    UnloadTexture(wheelRearTexture);
}

void Player::Update(float delta) 
{
    // Horizontal movement: A/D keys
    if (IsKeyDown(KEY_A)) position.x -= PLAYER_HOR_SPD * delta;
    if (IsKeyDown(KEY_D)) position.x += PLAYER_HOR_SPD * delta;

    // Jump
    if (IsKeyDown(KEY_SPACE) && canJump) 
    {
        velocity.y = -PLAYER_JUMP_SPD;
        canJump = false;
    }

    // Apply gravity
    velocity.y += GRAVITY * delta;
    position.y += velocity.y * delta;

    // Ground collision
    if (position.y >= GROUND_Y) 
    {
        position.y = GROUND_Y;
        velocity.y = 0;
        canJump = true;
    }
}

void Player::Reset(Vector2 startPos) 
{
    position = startPos;
    velocity = {0.0f, 0.0f};
    canJump = false;
    startX = startPos.x;
}

float Player::GetDistance() const 
{
    return position.x - startX;
}

void Player::Draw() const 
{
    float scale = 0.33f; // Scale factor for drawing

    DrawTextureEx(wheelRearTexture, Vector2{position.x - 180*scale, position.y + 5*scale}, 0.0f, scale, WHITE);
    DrawTextureEx(wheelFrontTexture, Vector2{position.x + 40*scale, position.y - 5*scale}, 0.0f, scale, WHITE);
    DrawTextureEx(bodyTexture, Vector2{position.x - (bodyTexture.width/2.0f)*scale, position.y - (bodyTexture.height/2.0f)*scale}, 0.0f, scale, WHITE);

}
