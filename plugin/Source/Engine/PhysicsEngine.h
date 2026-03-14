#pragma once

#include "State.h"
#include <vector>

class PhysicsEngine
{
public:
    // Run one physics tick. Returns events (note triggers).
    std::vector<PhysicsEvent> tick(PersistentState& persistent,
                                   TransientState& transient,
                                   float dt = 1.0f);

private:
    static constexpr int   TRAIL_MAX     = 60;
    static constexpr int   LOCKOUT_TICKS = 12;
    static constexpr float FLASH_DECAY   = 0.04f;
    static constexpr float EPSILON       = 0.001f;
    static constexpr float MAX_SPEED     = 0.025f;
};
