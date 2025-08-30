#include "Player.h"
#include <raylib.h>
#include <raymath.h>

constexpr float GRAVITY = 800.0f;
constexpr float PLAYER_HOR_SPD = 200.0f;
constexpr float PLAYER_JUMP_SPD = 350.0f;
constexpr int GROUND_Y = 400; // y-coordinate of ground

Player::Player(Vector2 startPos)
{
	position = startPos;
	velocity = { 0.0f, 0.0f };
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
	rearWheel.center  = { position.x - 42.5f, position.y + 20.0f };
    rearWheel.radius  = 14.5f;

    frontWheel.center = { position.x + 33.0f, position.y + 18.0f };
    frontWheel.radius = 17.0f;

    head.center = { position.x, position.y - 35.0f };
    head.radius = 7.5f;

    bodyPoly.vertices = 
	{
        { position.x - 55.0f, position.y - 15.0f },
        { position.x + 35.0f, position.y - 15.0f },
        { position.x,          position.y + 25.0f }
	};
}

void Player::Update(float delta, const Terrain& Terrain)
{
    // -------- INPUT: rear-wheel throttle --------
    if (IsKeyDown(KEY_W))      acceleration = 800.0f;   // accelerate
    else if (IsKeyDown(KEY_S)) acceleration = -400.0f;  // brake/reverse
    else                       acceleration = 0.0f;

    // -------- Horizontal dynamics (rear-wheel drives) --------
    velocity.x += acceleration * delta;         // integrate accel
    velocity.x *= 0.98f;                        // damping/friction
    position.x += velocity.x * delta;           // integrate vel

    // -------- Wheel spin from distance traveled --------
    UpdateCollisionShapes();                    // refresh radii before using them
    float distX = velocity.x * delta;
    float rearCirc  = 2.0f * PI * rearWheel.radius;
    float frontCirc = 2.0f * PI * frontWheel.radius;

    wheelRotationRear  += (distX / rearCirc)  * 360.0f;   // rear driven
    wheelRotationFront += (distX / frontCirc) * 360.0f;   // passive roll
    // keep angles in a small range (optional)
    if (wheelRotationRear  >  36000.0f || wheelRotationRear  < -36000.0f)  wheelRotationRear  = fmodf(wheelRotationRear, 360.0f);
    if (wheelRotationFront >  36000.0f || wheelRotationFront < -36000.0f) wheelRotationFront = fmodf(wheelRotationFront, 360.0f);

    velocity.y += 800.0f * delta;              // gravity
    position.y += velocity.y * delta;

    UpdateCollisionShapes();
    // Query terrain height under wheel centers
    float groundAtRear  = Terrain.GetHeightAt(rearWheel.center.x);
    float groundAtFront = Terrain.GetHeightAt(frontWheel.center.x);


    // compute penetration
    float penRear  = rearWheel.center.y  + rearWheel.radius  - groundAtRear;
    float penFront = frontWheel.center.y + frontWheel.radius - groundAtFront;

    // resolve once using max penetration
    float penetration = std::max(0.0f, std::max(penRear, penFront));
    if (penetration > 0.0f) {
        position.y -= penetration;
        velocity.y = 0.0f;
        UpdateCollisionShapes(); // recompute wheel centers
    }

    //float penetration = 0.0f;
    if (penRear  > penetration) penetration = penRear;
    if (penFront > penetration) penetration = penFront;

    if (penetration > 0.0f) {
        position.y -= penetration;     // lift bike once
        velocity.y = 0.0f;
        UpdateCollisionShapes();       // positions changed
    }
}

void Player::Reset(Vector2 startPos)
{
	position = startPos;
	velocity = { 0.0f, 0.0f };
	startX = startPos.x;
}

float Player::GetDistance() const
{
	return position.x - startX;
}

void Player::Draw() const
{
	// --- Wheels: draw centered on their collision circles ---
    Rectangle srcRear  = { 0, 0, (float)wheelRearTexture.width,  (float)wheelRearTexture.height };
    Rectangle srcFront = { 0, 0, (float)wheelFrontTexture.width, (float)wheelFrontTexture.height };

    Rectangle dstRear  = { rearWheel.center.x,  rearWheel.center.y,
                           wheelRearTexture.width  * scale, wheelRearTexture.height  * scale };
    Rectangle dstFront = { frontWheel.center.x, frontWheel.center.y,
                           wheelFrontTexture.width * scale, wheelFrontTexture.height * scale };

    Vector2 originRear  = { dstRear.width  * 0.5f, dstRear.height  * 0.5f };
    Vector2 originFront = { dstFront.width * 0.5f, dstFront.height * 0.5f };

    DrawTexturePro(wheelRearTexture,  srcRear,  dstRear,  originRear,  wheelRotationRear,  WHITE);
    DrawTexturePro(wheelFrontTexture, srcFront, dstFront, originFront, wheelRotationFront, WHITE);

    // --- Body: keep your current placement (tweak as needed) ---
    DrawTextureEx(bodyTexture,
        { position.x - (bodyTexture.width  * 0.5f) * scale,
          position.y - (bodyTexture.height * 0.5f) * scale },
        0.0f, scale, WHITE);

    // --- Debug (optional) ---
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