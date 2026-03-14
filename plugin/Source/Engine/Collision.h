#pragma once

#include <cmath>
#include <algorithm>

enum class RectFace { Left, Right, Top, Bottom };

struct CircleCircleResult
{
    bool overlapping = false;
    float dist = 0.0f;
    float dx = 0.0f;
    float dy = 0.0f;
    float minDist = 0.0f;
};

namespace Collision
{
    bool circleRectOverlap(float bx, float by, float radius,
                           float cx, float cy, float hw, float hh);

    RectFace minOverlapFace(float bx, float by, float radius,
                            float cx, float cy, float hw, float hh);

    CircleCircleResult circleCircleOverlap(float x1, float y1, float r1,
                                           float x2, float y2, float r2);
}
