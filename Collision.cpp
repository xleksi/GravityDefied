#include "Collision.h"
#include <cmath>
#include <algorithm>

// ==== Circle vs Rectangle ====
bool CheckCircleRect(const Circle& c, const Rectangle& r) {
    // Clamp circle center to rectangle bounds
    float closestX = fmaxf(r.x, fminf(c.center.x, r.x + r.width));
    float closestY = fmaxf(r.y, fminf(c.center.y, r.y + r.height));

    float dx = c.center.x - closestX;
    float dy = c.center.y - closestY;

    return (dx*dx + dy*dy) <= (c.radius * c.radius);
}

// ==== Circle vs Circle ====
bool CheckCircleCircle(const Circle& a, const Circle& b) {
    float dx = a.center.x - b.center.x;
    float dy = a.center.y - b.center.y;
    float distSq = dx*dx + dy*dy;
    float radiusSum = a.radius + b.radius;
    return distSq <= (radiusSum * radiusSum);
}

// ==== Point in Polygon (ray casting) ====
bool PointInPolygon(Vector2 p, const Polygon& poly) {
    bool inside = false;
    for (size_t i = 0, j = poly.vertices.size() - 1; i < poly.vertices.size(); j = i++) {
        Vector2 vi = poly.vertices[i];
        Vector2 vj = poly.vertices[j];

        if (((vi.y > p.y) != (vj.y > p.y)) &&
            (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y) + vi.x)) {
            inside = !inside;
        }
    }
    return inside;
}

// ==== Polygon vs Rectangle (basic check) ====
bool CheckPolygonRect(const Polygon& p, const Rectangle& r) {
    // Check if any polygon vertex is inside rect
    for (auto& v : p.vertices) {
        if (v.x >= r.x && v.x <= r.x + r.width &&
            v.y >= r.y && v.y <= r.y + r.height) {
            return true;
        }
    }
    // Check if rect corners are inside polygon
    Vector2 corners[4] = {
        {r.x, r.y},
        {r.x + r.width, r.y},
        {r.x + r.width, r.y + r.height},
        {r.x, r.y + r.height}
    };
    for (auto& c : corners) {
        if (PointInPolygon(c, p)) return true;
    }

    return false;
}
