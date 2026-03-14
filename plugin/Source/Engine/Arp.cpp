#include "Arp.h"
#include <algorithm>
#include <cmath>

std::vector<int> ArpEngine::buildSequence(const std::vector<Zone>& zones, ArpPlayMode mode)
{
    if (zones.empty()) return {};

    std::vector<int> indices;
    indices.reserve(zones.size());
    for (int i = 0; i < static_cast<int>(zones.size()); i++)
        indices.push_back(i);

    switch (mode)
    {
        case ArpPlayMode::FirstPlaced:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].placementOrder < zones[b].placementOrder;
            });
            break;

        case ArpPlayMode::Up:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].midi < zones[b].midi;
            });
            break;

        case ArpPlayMode::Down:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].midi > zones[b].midi;
            });
            break;

        case ArpPlayMode::LeftRight:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].cx < zones[b].cx;
            });
            break;

        case ArpPlayMode::RightLeft:
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].cx > zones[b].cx;
            });
            break;

        case ArpPlayMode::VerticalUp:
            // Bottom-most first (largest cy)
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].cy > zones[b].cy;
            });
            break;

        case ArpPlayMode::VerticalDown:
            // Top-most first (smallest cy)
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return zones[a].cy < zones[b].cy;
            });
            break;

        case ArpPlayMode::Random:
            // Order doesn't matter for random
            break;

        default:
            break;
    }

    return indices;
}

void ArpEngine::maybeRebuildSequence(const std::vector<Zone>& zones, ArpPlayMode mode, ArpState& arp)
{
    int count = static_cast<int>(zones.size());

    if (arp.needsRebuild || count != arp.lastZoneCount)
    {
        arp.sequence = buildSequence(zones, mode);
        arp.lastZoneCount = count;
        arp.needsRebuild = false;

        if (!arp.sequence.empty())
            arp.sequencePos = arp.sequencePos % static_cast<int>(arp.sequence.size());
        else
            arp.sequencePos = 0;
    }
}

void ArpEngine::advancePosition(ArpState& arp, ArpPlayMode mode, bool pendulum, PRNG& prng)
{
    int len = static_cast<int>(arp.sequence.size());
    if (len <= 1) return;

    if (mode == ArpPlayMode::Random)
    {
        int next;
        do {
            next = static_cast<int>(prng() * len) % len;
        } while (next == arp.sequencePos && len > 1);
        arp.sequencePos = next;
        return;
    }

    if (pendulum)
    {
        int nextPos = arp.sequencePos + arp.direction;
        if (nextPos >= len)
        {
            arp.direction = -1;
            arp.sequencePos = len - 2;
            if (arp.sequencePos < 0) arp.sequencePos = 0;
        }
        else if (nextPos < 0)
        {
            arp.direction = 1;
            arp.sequencePos = 1;
            if (arp.sequencePos >= len) arp.sequencePos = 0;
        }
        else
        {
            arp.sequencePos = nextPos;
        }
    }
    else
    {
        arp.sequencePos = (arp.sequencePos + 1) % len;
    }
}

std::vector<ArpEngine::ArpEvent> ArpEngine::processBlock(
    PersistentState& persistent,
    TransientState& transient,
    int numSamples,
    double bpm,
    double sampleRate)
{
    std::vector<ArpEvent> events;

    if (!persistent.arpEnabled || persistent.zones.empty() || bpm <= 0.0)
        return events;

    auto& arp = transient.arpState;

    maybeRebuildSequence(persistent.zones,
                         static_cast<ArpPlayMode>(persistent.arpPlayMode),
                         arp);

    if (arp.sequence.empty()) return events;

    // Calculate interval in samples
    double intervalSamples;
    if (persistent.arpSync)
    {
        intervalSamples = divisionToSamples(
            static_cast<ArpDivision>(persistent.arpDivision), bpm, sampleRate);
    }
    else
    {
        intervalSamples = persistent.arpRateMs * sampleRate / 1000.0;
    }

    int ratchetTotal = persistent.arpRatchet;
    int hitsPerStep = ratchetTotal + 1;
    double hitInterval = intervalSamples / hitsPerStep;

    // Walk through the buffer looking for trigger points
    for (int s = 0; s < numSamples; s++)
    {
        arp.sampleAccumulator += 1.0;

        if (arp.sampleAccumulator >= hitInterval)
        {
            arp.sampleAccumulator -= hitInterval;

            int zoneIndex = arp.sequence[arp.sequencePos];
            if (zoneIndex >= 0 && zoneIndex < static_cast<int>(persistent.zones.size()))
            {
                auto& zone = persistent.zones[zoneIndex];
                events.push_back({ zone.midi, 0.8f, zoneIndex, s });

                // Flash
                if (zoneIndex < static_cast<int>(transient.zoneFlash.size()))
                    transient.zoneFlash[zoneIndex] = 1.0f;
            }

            // Advance
            if (ratchetTotal > 0)
            {
                arp.ratchetCount++;
                if (arp.ratchetCount >= ratchetTotal)
                {
                    arp.ratchetCount = 0;
                    advancePosition(arp,
                                    static_cast<ArpPlayMode>(persistent.arpPlayMode),
                                    persistent.arpPendulum,
                                    transient.prng);
                }
            }
            else
            {
                advancePosition(arp,
                                static_cast<ArpPlayMode>(persistent.arpPlayMode),
                                persistent.arpPendulum,
                                transient.prng);
            }
        }
    }

    return events;
}
