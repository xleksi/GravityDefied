#pragma once
#include "raylib.h"
#include <vector>

class Terrain {
public:
    Terrain(float startX = 0.0f);

    // (Re)generate points covering [cameraX - viewBehind, cameraX + viewAhead]
    void EnsureCoverage(float minX, float maxX);

    // Call each frame to maybe extend/remove points based on camera/player
    void Update(float cameraX);

    // Draw using provided camera
    void Draw() const;

    // Query height (y) at arbitrary world x (linear interp)
    float GetHeightAt(float x) const;

    // Settings you may tweak
    float baseY;           // baseline y coordinate
    float segmentLength;   // horizontal distance between samples
    float bumpRange;       // max vertical change per sample
    float viewAhead;       // generation distance ahead of camera
    float viewBehind;      // generation distance behind camera

private:
    std::vector<Vector2> pts; // sampled points (x,y)
    float leftX;              // current leftmost x covered
    float rightX;             // current rightmost x covered

    void GenerateNext();      // generate one new point at right side
    void GeneratePrev();      // (not used often) generate to the left if needed
};
