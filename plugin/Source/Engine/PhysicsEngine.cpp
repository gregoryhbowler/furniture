#include "PhysicsEngine.h"
#include "Collision.h"
#include "Scales.h"
#include <cmath>
#include <algorithm>

// Fan direction enum values matching State.h: 0=N, 1=S, 2=E, 3=W, 4=Random
enum FanDir { North = 0, South, East, West, Random };

std::vector<PhysicsEvent> PhysicsEngine::tick(PersistentState& persistent,
                                               TransientState& transient,
                                               float dt)
{
    std::vector<PhysicsEvent> events;
    auto& balls = transient.balls;
    auto& zoneFlash = transient.zoneFlash;
    auto& zoneLockout = transient.zoneLockout;
    auto& prng = transient.prng;
    auto& zones = persistent.zones;

    transient.tickCount += dt;

    const float gravityVal  = (persistent.gravity / 100.0f) * 0.0003f;
    const float frictionVal = (persistent.friction / 100.0f) * 0.03f;
    const float bounceVal   = std::min(persistent.bounce / 100.0f, 1.0f);
    const float wallBounceVal = bounceVal;
    const float ballRadius  = 0.005f + (persistent.ballSize / 18.0f) * 0.015f;
    const float minEnergyVal = (persistent.minEnergy / 100.0f) * 0.004f;
    const float momentumRaw = persistent.momentum / 100.0f;

    // Jitter: cubic curve so low values are subtle
    const float jitterNorm = persistent.jitter / 100.0f;
    const float jitterVal  = jitterNorm * jitterNorm * jitterNorm * 0.003f;

    // Fan
    const float fanForceVal = (persistent.fanAmount / 100.0f) * 0.0004f;
    const float fanFreq     = (persistent.fanSpeed / 100.0f) * 0.15f;
    const float fanPhase    = transient.tickCount * fanFreq;
    const float fanOsc      = std::sin(fanPhase);

    const auto& scaleNameStr = Scales::getAllScales()[persistent.scaleName].name;

    for (size_t b = 0; b < balls.size(); b++)
    {
        auto& ball = balls[b];

        // 1. Gravity
        ball.vy += gravityVal * dt;

        // 2. Fan force
        if (fanForceVal > 0.0f)
        {
            float force = fanOsc * fanForceVal * dt;
            switch (persistent.fanDirection)
            {
                case North:  ball.vy -= std::abs(force); break;
                case South:  ball.vy += std::abs(force); break;
                case East:   ball.vx += std::abs(force); break;
                case West:   ball.vx -= std::abs(force); break;
                case Random: {
                    float angle = transient.tickCount * fanFreq * 0.3f + b * 2.39996f;
                    ball.vx += std::cos(angle) * force;
                    ball.vy += std::sin(angle) * force;
                    break;
                }
            }
        }

        // 3. Jitter
        if (jitterVal > 0.0f)
        {
            ball.vx += (prng() - 0.5f) * 2.0f * jitterVal * dt;
            ball.vy += (prng() - 0.5f) * 2.0f * jitterVal * dt;
        }

        // 4. Friction
        float fricMul = std::pow(1.0f - frictionVal, dt);
        ball.vx *= fricMul;
        ball.vy *= fricMul;

        // 5. Minimum energy enforcement
        float spdAfterFric = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
        if (spdAfterFric < minEnergyVal && minEnergyVal > 0.0f)
        {
            if (spdAfterFric > 0.00001f)
            {
                float scale = minEnergyVal / spdAfterFric;
                ball.vx *= scale;
                ball.vy *= scale;
            }
            else
            {
                float angle = prng() * 3.14159265f * 2.0f;
                ball.vx = std::cos(angle) * minEnergyVal;
                ball.vy = std::sin(angle) * minEnergyVal;
            }
        }

        // 6. Position update
        ball.x += ball.vx * dt;
        ball.y += ball.vy * dt;

        // 7. Wall bouncing
        if (ball.x - ballRadius < 0.0f)
        {
            ball.x = ballRadius + EPSILON;
            ball.vx = std::abs(ball.vx) * wallBounceVal;
        }
        else if (ball.x + ballRadius > 1.0f)
        {
            ball.x = 1.0f - ballRadius - EPSILON;
            ball.vx = -std::abs(ball.vx) * wallBounceVal;
        }
        if (ball.y - ballRadius < 0.0f)
        {
            ball.y = ballRadius + EPSILON;
            ball.vy = std::abs(ball.vy) * wallBounceVal;
        }
        else if (ball.y + ballRadius > 1.0f)
        {
            ball.y = 1.0f - ballRadius - EPSILON;
            ball.vy = -std::abs(ball.vy) * wallBounceVal;
        }

        // 7b. Hard velocity cap
        float spdNow = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
        if (spdNow > MAX_SPEED)
        {
            float cs = MAX_SPEED / spdNow;
            ball.vx *= cs;
            ball.vy *= cs;
        }

        // 8. Zone collision
        for (size_t z = 0; z < zones.size(); z++)
        {
            if (zoneLockout[z] > 0.0f) continue;

            auto& zone = zones[z];
            if (Collision::circleRectOverlap(ball.x, ball.y, ballRadius,
                                             zone.cx, zone.cy, zone.hw, zone.hh))
            {
                float impactSpeed = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
                float velocity = std::clamp(impactSpeed / 0.012f, 0.3f, 1.0f);

                int outMidi = zone.midi;

                // Random pitch
                if (persistent.randomPitch > 0 && prng() * 100.0f < persistent.randomPitch)
                {
                    auto scaleNotes = Scales::getScaleNotesInOctave(outMidi, persistent.scaleRoot, scaleNameStr);
                    if (!scaleNotes.empty())
                        outMidi = scaleNotes[static_cast<int>(prng() * scaleNotes.size()) % scaleNotes.size()];
                }

                // Random octave
                if (persistent.randomOctaveChance > 0 && prng() * 100.0f < persistent.randomOctaveChance)
                {
                    int maxShift = persistent.randomOctaveAmount;
                    int shift = static_cast<int>(prng() * maxShift * 2 + 1) - maxShift;
                    if (shift == 0) shift = prng() < 0.5f ? -1 : 1;
                    int shifted = outMidi + shift * 12;
                    if (shifted >= 12 && shifted <= 120)
                        outMidi = shifted;
                }

                // Random velocity
                float outVelocity = velocity;
                if (persistent.randomVelocity > 0)
                {
                    float velRange = persistent.randomVelocity / 100.0f;
                    outVelocity = velocity * (1.0f - velRange) + prng() * velocity * velRange * 2.0f;
                    outVelocity = std::clamp(outVelocity, 0.1f, 1.0f);
                }

                events.push_back({ outMidi, outVelocity, static_cast<int>(z) });

                zoneFlash[z] = 1.0f;
                zoneLockout[z] = LOCKOUT_TICKS;

                // Solid zone bounce with momentum
                if (persistent.solidZones)
                {
                    auto face = Collision::minOverlapFace(ball.x, ball.y, ballRadius,
                                                         zone.cx, zone.cy, zone.hw, zone.hh);
                    float speedRatio = std::min(1.0f, impactSpeed / MAX_SPEED);
                    float momentumBoost = momentumRaw * (1.0f - speedRatio * 0.8f);
                    float bounceMultiplier = bounceVal * (1.0f + momentumBoost);

                    switch (face)
                    {
                        case RectFace::Left:
                            ball.vx = -std::abs(ball.vx) * bounceMultiplier;
                            ball.x = zone.cx - zone.hw - ballRadius - EPSILON;
                            break;
                        case RectFace::Right:
                            ball.vx = std::abs(ball.vx) * bounceMultiplier;
                            ball.x = zone.cx + zone.hw + ballRadius + EPSILON;
                            break;
                        case RectFace::Top:
                            ball.vy = -std::abs(ball.vy) * bounceMultiplier;
                            ball.y = zone.cy - zone.hh - ballRadius - EPSILON;
                            break;
                        case RectFace::Bottom:
                            ball.vy = std::abs(ball.vy) * bounceMultiplier;
                            ball.y = zone.cy + zone.hh + ballRadius + EPSILON;
                            break;
                    }

                    // Cap after momentum bounce
                    float postSpd = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
                    if (postSpd > MAX_SPEED)
                    {
                        float cs = MAX_SPEED / postSpd;
                        ball.vx *= cs;
                        ball.vy *= cs;
                    }
                }
            }
        }

        // 9. Trail
        ball.trail.push_back({ ball.x, ball.y });
        if (ball.trail.size() > TRAIL_MAX)
            ball.trail.erase(ball.trail.begin());
    }

    // 10. Ball-ball collision
    if (persistent.ballCollide)
    {
        for (size_t i = 0; i < balls.size(); i++)
        {
            for (size_t j = i + 1; j < balls.size(); j++)
            {
                auto& a = balls[i];
                auto& b2 = balls[j];
                auto result = Collision::circleCircleOverlap(a.x, a.y, ballRadius,
                                                             b2.x, b2.y, ballRadius);
                if (result.overlapping && result.dist > 0.0001f)
                {
                    float nx = result.dx / result.dist;
                    float ny = result.dy / result.dist;

                    float dvx = a.vx - b2.vx;
                    float dvy = a.vy - b2.vy;
                    float dvn = dvx * nx + dvy * ny;

                    if (dvn > 0.0f)
                    {
                        a.vx  -= dvn * nx;
                        a.vy  -= dvn * ny;
                        b2.vx += dvn * nx;
                        b2.vy += dvn * ny;
                    }

                    float overlap = result.minDist - result.dist;
                    float pushX = (overlap / 2.0f + EPSILON) * nx;
                    float pushY = (overlap / 2.0f + EPSILON) * ny;
                    a.x  -= pushX;
                    a.y  -= pushY;
                    b2.x += pushX;
                    b2.y += pushY;
                }
            }
        }
    }

    // Decay flash and lockout
    for (size_t z = 0; z < zones.size(); z++)
    {
        if (zoneFlash[z] > 0.0f)
            zoneFlash[z] = std::max(0.0f, zoneFlash[z] - FLASH_DECAY * dt);
        if (zoneLockout[z] > 0.0f)
            zoneLockout[z] = std::max(0.0f, zoneLockout[z] - dt);
    }

    return events;
}
