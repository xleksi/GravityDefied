#pragma once
#include <raylib.h>

static inline float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

bool IsPointBelowLine(Vector2 a, Vector2 b, Vector2 point, Vector2* collisionPoint);
