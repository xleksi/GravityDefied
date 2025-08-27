#include "Terrain.h"
#include <random>
#include <algorithm>
#include <ctime>

Terrain::Terrain(float startX) {
    baseY = 400.0f;       // default baseline (tweak to your camera/world)
    segmentLength = 40.0f; // horizontal sample spacing - tweak smaller for bumpier
    bumpRange = 40.0f;     // per-sample vertical change limit
    viewAhead = 1200.0f;
    viewBehind = 800.0f;

    pts.clear();
    leftX = startX - viewBehind;
    rightX = startX + viewAhead;

    // seed initial linear points
    float x = leftX;
    std::mt19937 rng(12345); // fixed seed for repeatable testing; replace with device seed later
    std::uniform_real_distribution<float> d(-bumpRange, bumpRange);

    // build initial points using small random walk
    float y = baseY;
    while (x <= rightX + segmentLength) {
        y += d(rng) * 0.25f;                     // small jitter
        // clamp to reasonable range
        y = std::clamp(y, baseY - 120.0f, baseY + 120.0f);
        pts.push_back({ x, y });
        x += segmentLength;
    }
    // set exact bounds
    leftX = pts.front().x;
    rightX = pts.back().x;
}

// Generate a next sample on the right
void Terrain::GenerateNext() {
    // pseudo-random using previous value (deterministic but ok)
    static std::mt19937 rng( (unsigned) time(nullptr) );
    std::uniform_real_distribution<float> d(-bumpRange, bumpRange);

    float lastY = pts.empty() ? baseY : pts.back().y;
    float nx = pts.empty() ? leftX : pts.back().x + segmentLength;
    float ny = lastY + d(rng) * 0.5f; // smaller steps
    ny = std::clamp(ny, baseY - 160.0f, baseY + 160.0f);
    pts.push_back({ nx, ny });
    rightX = pts.back().x;
}

// Extend left if needed (usually not needed but implemented)
void Terrain::GeneratePrev() {
    static std::mt19937 rng( (unsigned) (time(nullptr) ^ 0xdeadbeef) );
    std::uniform_real_distribution<float> d(-bumpRange, bumpRange);

    float firstY = pts.empty() ? baseY : pts.front().y;
    float nx = pts.empty() ? leftX : pts.front().x - segmentLength;
    float ny = firstY + d(rng) * 0.5f;
    ny = std::clamp(ny, baseY - 160.0f, baseY + 160.0f);
    pts.insert(pts.begin(), { nx, ny });
    leftX = pts.front().x;
}

void Terrain::EnsureCoverage(float minX, float maxX) {
    // extend to the right until we cover maxX
    while (rightX < maxX + segmentLength) GenerateNext();
    // extend to the left if needed
    while (leftX > minX - segmentLength) GeneratePrev();

    // trim far-left points to keep vector small
    while (!pts.empty() && pts.size() > 4 && pts.front().x < minX - segmentLength*2) {
        pts.erase(pts.begin());
        leftX = pts.front().x;
    }
    // trim far-right (rare)
    while (!pts.empty() && pts.size() > 4 && pts.back().x > maxX + segmentLength*3) {
        pts.pop_back();
        rightX = pts.back().x;
    }
}

void Terrain::Update(float cameraX) {
    float minX = cameraX - viewBehind;
    float maxX = cameraX + viewAhead;
    EnsureCoverage(minX, maxX);
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float Terrain::GetHeightAt(float x) const {
    if (pts.empty()) return baseY;
    // clamp outside
    if (x <= pts.front().x) return pts.front().y;
    if (x >= pts.back().x)  return pts.back().y;
    // find segment
    // naive linear scan (fast enough for small arrays); could do binary search if many points
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        float x1 = pts[i].x, x2 = pts[i+1].x;
        if (x >= x1 && x <= x2) {
            float t = (x - x1) / (x2 - x1);
            // smooth interpolation (optional): use cosine interp for smoother hills
            float t2 = (1 - cosf(t * PI)) * 0.5f;
            return lerp(pts[i].y, pts[i+1].y, t2);
        }
    }
    return baseY;
}

void Terrain::Draw() const {
    if (pts.size() < 2) return;
    // fill polygon between samples and bottom of screen (big Y value)
    float bottom = baseY + 600.0f; // deep enough to cover screen
    Color fill = DARKGREEN;
    // draw triangles for each segment
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        Vector2 p1 = pts[i];
        Vector2 p2 = pts[i+1];
        // two triangles: p1,p2,bottom-right and p1,bottom-right,bottom-left
        DrawTriangle(p1, p2, { p2.x, bottom }, fill);
        DrawTriangle(p1, { p2.x, bottom }, { p1.x, bottom }, fill);
        // optionally draw ridge line
        DrawLineEx(p1, p2, 3.0f, GREEN);
    }
}
