#include "Terrain.h"
#include "Config.h"
#include <cstdlib> // for rand()
#include <cmath>

inline float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

void Terrain::GenerateInitial(float startX, float width, float segmentMean, int randomOffset, int heightRandom, float jumpProbability, float minY, float maxY) {
    points.clear();

    float curX = startX;
    float posY = (minY + maxY) * 0.5f; // start at middle of allowed range
    int initialSegments = 30;

    for (int i = 0; i < initialSegments; ++i) {
        float nextX = curX + segmentMean + (rand() % (2 * randomOffset) - randomOffset);
        int moveY = ((float)rand() / RAND_MAX) < jumpProbability ? -(rand() % (heightRandom * 2) + heightRandom) 
                                                                  : rand() % (2 * heightRandom) - heightRandom;
        int nextY = (int)clampf(posY + moveY, minY, maxY);

        points.push_back({nextX, (float)nextY});
        posY = nextY;
        curX = nextX;
    }
}

void Terrain::GenerateAhead(float cameraX, float generateAhead, float segmentMean, int randomOffset, int heightRandom, float jumpProbability) {
    while (points.empty() || points.back().x < cameraX + generateAhead) {
        Vector2 last = points.empty() ? Vector2{cameraX, (float)WINDOW_HEIGHT / 2} : points.back();
        int moveY = rand() % (2 * heightRandom) - heightRandom;
        float nextX = last.x + segmentMean + (rand() % (2 * randomOffset) - randomOffset);
        float nextY = clampf(last.y + moveY, WINDOW_HEIGHT * 0.25f, WINDOW_HEIGHT * 0.95f);
        points.push_back({nextX, nextY});
    }
}

void Terrain::TrimBehind(float cameraX, float removalPadding) {
    while (points.size() > 6 && points.front().x < cameraX - removalPadding) {
        points.erase(points.begin());
    }
}

void Terrain::Draw() {
    for (size_t i = 1; i < points.size(); ++i) {
        DrawLineEx(points[i - 1], points[i], 5, BLACK);
    }
}
