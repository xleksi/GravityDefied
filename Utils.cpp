#include "Utils.h"
#include <cmath>
#include "Config.h"
#include "Utils.h"


bool IsPointBelowLine(Vector2 a, Vector2 b, Vector2 point, Vector2* collisionPoint) {
    if (fabsf(b.x - a.x) > 1e-6f) {
        float t = (point.x - a.x) / (b.x - a.x);
        if (t < 0.0f || t > 1.0f) return false;

        collisionPoint->x = point.x;
        collisionPoint->y = a.y + t * (b.y - a.y);
        return (point.y > collisionPoint->y);
    } else {
        if (fabsf(point.x - a.x) > 1e-6f) return false;
        collisionPoint->x = a.x;
        collisionPoint->y = fmaxf(a.y, b.y);
        return (point.y > collisionPoint->y);
    }
}
