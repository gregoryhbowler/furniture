#include "Collision.h"

namespace Collision
{

bool circleRectOverlap(float bx, float by, float radius,
                       float cx, float cy, float hw, float hh)
{
    float nearestX = std::clamp(bx, cx - hw, cx + hw);
    float nearestY = std::clamp(by, cy - hh, cy + hh);
    float dx = bx - nearestX;
    float dy = by - nearestY;
    return (dx * dx + dy * dy) < (radius * radius);
}

RectFace minOverlapFace(float bx, float by, float radius,
                        float cx, float cy, float hw, float hh)
{
    float overlapLeft  = (bx + radius) - (cx - hw);
    float overlapRight = (cx + hw) - (bx - radius);
    float overlapTop   = (by + radius) - (cy - hh);
    float overlapBot   = (cy + hh) - (by - radius);

    float minVal = overlapLeft;
    RectFace face = RectFace::Left;

    if (overlapRight < minVal) { minVal = overlapRight; face = RectFace::Right; }
    if (overlapTop < minVal)   { minVal = overlapTop;   face = RectFace::Top; }
    if (overlapBot < minVal)   { face = RectFace::Bottom; }

    return face;
}

CircleCircleResult circleCircleOverlap(float x1, float y1, float r1,
                                       float x2, float y2, float r2)
{
    CircleCircleResult result;
    result.dx = x2 - x1;
    result.dy = y2 - y1;
    result.dist = std::sqrt(result.dx * result.dx + result.dy * result.dy);
    result.minDist = r1 + r2;
    result.overlapping = result.dist < result.minDist;
    return result;
}

} // namespace Collision
