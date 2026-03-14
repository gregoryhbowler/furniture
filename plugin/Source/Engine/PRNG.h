#pragma once

#include <cstdint>

// Mulberry32 seeded PRNG — deterministic, portable
// Direct port of js/engine/prng.js
class PRNG
{
public:
    explicit PRNG(uint32_t seed = 42) : state(seed) {}

    void reset(uint32_t seed = 42) { state = seed; }

    // Returns float in [0, 1)
    float next()
    {
        state += 0x6D2B79F5u;
        uint32_t t = state;
        t = (t ^ (t >> 15)) * (t | 1u);
        t ^= t + (t ^ (t >> 7)) * (t | 61u);
        return static_cast<float>((t ^ (t >> 14)) >> 0) / 4294967296.0f;
    }

    float operator()() { return next(); }

private:
    uint32_t state;
};
