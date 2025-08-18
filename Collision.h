#pragma once
#include "raylib.h"
#include <vector>

// ==== Shape structures ====
struct Circle {
    Vector2 center;
    float radius;
};

struct Polygon {
    std::vector<Vector2> vertices;
};

// ==== Collision functions ====

// Circle vs Rectangle (axis-aligned)
bool CheckCircleRect(const Circle& c, const Rectangle& r);

// Circle vs Circle (basic overlap check)
bool CheckCircleCircle(const Circle& a, const Circle& b);

// Point in Polygon (helper, useful for SAT later)
bool PointInPolygon(Vector2 p, const Polygon& poly);

// Polygon vs Rectangle (basic overlap check)
bool CheckPolygonRect(const Polygon& p, const Rectangle& r);
