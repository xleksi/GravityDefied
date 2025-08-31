#pragma once
#include <vector>
#include <raylib.h>

class Terrain {
public:
    std::vector<Vector2> points;

    void GenerateInitial(float startX, float width, float segmentMean, int randomOffset, int heightRandom, float jumpProbability, float minY, float maxY);
    void GenerateAhead(float cameraX, float generateAhead, float segmentMean, int randomOffset, int heightRandom, float jumpProbability);
    void TrimBehind(float cameraX, float removalPadding);
    void Draw();
    int FindSegment(float x) const;
};
