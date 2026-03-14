// Rectangle-circle and circle-circle collision math
// PURE — no DOM, no browser APIs

// Find nearest point on axis-aligned rectangle to point (px, py)
// Rectangle defined by center (cx, cy) and half-widths (hw, hh)
export function nearestPointOnRect(px, py, cx, cy, hw, hh) {
  const nx = Math.max(cx - hw, Math.min(px, cx + hw));
  const ny = Math.max(cy - hh, Math.min(py, cy + hh));
  return { x: nx, y: ny };
}

// Check if circle (bx, by, radius) overlaps rectangle (cx, cy, hw, hh)
export function circleRectOverlap(bx, by, radius, cx, cy, hw, hh) {
  const nearest = nearestPointOnRect(bx, by, cx, cy, hw, hh);
  const dx = bx - nearest.x;
  const dy = by - nearest.y;
  return (dx * dx + dy * dy) < (radius * radius);
}

// Determine which face of rectangle the circle penetrated least
// Returns 'left', 'right', 'top', or 'bottom'
export function minOverlapFace(bx, by, radius, cx, cy, hw, hh) {
  const overlapLeft   = (bx + radius) - (cx - hw);
  const overlapRight  = (cx + hw) - (bx - radius);
  const overlapTop    = (by + radius) - (cy - hh);
  const overlapBottom = (cy + hh) - (by - radius);

  const min = Math.min(overlapLeft, overlapRight, overlapTop, overlapBottom);

  if (min === overlapLeft)   return 'left';
  if (min === overlapRight)  return 'right';
  if (min === overlapTop)    return 'top';
  return 'bottom';
}

// Check if two circles overlap
export function circleCircleOverlap(x1, y1, r1, x2, y2, r2) {
  const dx = x2 - x1;
  const dy = y2 - y1;
  const dist = Math.sqrt(dx * dx + dy * dy);
  const minDist = r1 + r2;
  return { overlapping: dist < minDist, dist, dx, dy, minDist };
}
