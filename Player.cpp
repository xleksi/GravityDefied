#include "Player.h"
#include <raylib.h>

constexpr float GRAVITY = 800.0f;
constexpr float PLAYER_HOR_SPD = 200.0f;
constexpr float PLAYER_JUMP_SPD = 350.0f;
constexpr int GROUND_Y = 400; // y-coordinate of ground

Player::Player(Vector2 startPos)
{
	position = startPos;
	velocity = { 0.0f, 0.0f };
	canJump = false;
	startX = startPos.x;

	bodyTexture = LoadTexture("assets/body.png");
	wheelFrontTexture = LoadTexture("assets/wheel_front.png");
	wheelRearTexture = LoadTexture("assets/wheel_rear.png");

	if (bodyTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load body.png");
	if (wheelFrontTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load wheel_front.png");
	if (wheelRearTexture.id == 0) TraceLog(LOG_ERROR, "Failed to load wheel_rear.png");
}

void Player::UpdateCollisionShapes()
{
	// Rear wheel
	rearWheel.center = { position.x - 42.5f, position.y + 18.0f };
	rearWheel.radius = 14.5f;

	// Front wheel
	frontWheel.center = { position.x + 33.0f, position.y + 18.0f };
	frontWheel.radius = 17.0f;

	// Head
	head.center = { position.x, position.y - 35.0f };
	head.radius = 7.5f;

	// Body polygon
	bodyPoly.vertices = {
		{ position.x - 55.0f, position.y - 15.0f }, // left top
		{ position.x + 35.0f, position.y - 15.0f }, // right top
		{ position.x, position.y + 25.0f }         // bottom tip
	};
}

void Player::Update(float delta, const Rectangle& ground)
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

	// Ground collision (use provided ground rectangle)
	if (position.y >= ground.y)
	{
		position.y = ground.y;
		velocity.y = 0;
		canJump = true;
	}

	UpdateCollisionShapes();
}

void Player::Reset(Vector2 startPos)
{
	position = startPos;
	velocity = { 0.0f, 0.0f };
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

	DrawTextureEx(wheelRearTexture, Vector2{ position.x - 180.0f * scale, position.y + 5.0f * scale }, 0.0f, scale, WHITE);
	DrawTextureEx(wheelFrontTexture, Vector2{ position.x + 40.0f * scale, position.y - 5.0f * scale }, 0.0f, scale, WHITE);
	DrawTextureEx(bodyTexture, Vector2{ position.x - (bodyTexture.width / 2.0f) * scale, position.y - (bodyTexture.height / 2.0f) * scale }, 0.0f, scale, WHITE);

	// Debug draw collision shapes
	DrawCircleLines((int)rearWheel.center.x, (int)rearWheel.center.y, rearWheel.radius, RED);
	DrawCircleLines((int)frontWheel.center.x, (int)frontWheel.center.y, frontWheel.radius, RED);
	DrawCircleLines((int)head.center.x, (int)head.center.y, head.radius, BLUE);

	// Body polygon
	for (size_t i = 0; i < bodyPoly.vertices.size(); i++)
	{
		Vector2 v1 = bodyPoly.vertices[i];
		Vector2 v2 = bodyPoly.vertices[(i + 1) % bodyPoly.vertices.size()];
		DrawLineV(v1, v2, GREEN);
	}
}