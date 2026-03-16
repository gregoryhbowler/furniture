#include "Arp.h"
#include "Scales.h"
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

std::vector<int> ArpEngine::applyPath(const std::vector<int>& baseSeq, ArpPath path, int loopLen, PRNG& prng)
{
    if (baseSeq.empty()) return baseSeq;
    int n = static_cast<int>(baseSeq.size());

    switch (path)
    {
        case ArpPath::Sequential:
            return baseSeq;

        case ArpPath::Palindrome:
        {
            // Forward then reverse: 1-2-3-4-5-6-7-8, 8-7-6-5-4-3-2-1
            std::vector<int> result = baseSeq;
            for (int i = n - 1; i >= 0; i--)
                result.push_back(baseSeq[i]);
            return result;
        }

        case ArpPath::Interleaved:
        {
            // Interleave first half with second half: 1-5-2-6-3-7-4-8
            int half = (n + 1) / 2;
            std::vector<int> result;
            result.reserve(n);
            for (int i = 0; i < half; i++)
            {
                result.push_back(baseSeq[i]);
                int j = i + half;
                if (j < n)
                    result.push_back(baseSeq[j]);
            }
            return result;
        }

        case ArpPath::Zigzag:
        {
            // Zigzag within groups of 4: 1-3-2-4, 5-7-6-8
            std::vector<int> result;
            result.reserve(n);
            for (int g = 0; g < n; g += 4)
            {
                int end = std::min(g + 4, n);
                int groupSize = end - g;
                if (groupSize >= 3)
                {
                    result.push_back(baseSeq[g]);       // 1
                    result.push_back(baseSeq[g + 2]);   // 3
                    result.push_back(baseSeq[g + 1]);   // 2
                    if (groupSize >= 4)
                        result.push_back(baseSeq[g + 3]); // 4
                }
                else
                {
                    for (int i = g; i < end; i++)
                        result.push_back(baseSeq[i]);
                }
            }
            return result;
        }

        case ArpPath::Cross:
        {
            // Cross interleave: 1-5-2-7, 4-8-3-6
            int half = (n + 1) / 2;
            std::vector<int> first(baseSeq.begin(), baseSeq.begin() + half);
            std::vector<int> second(baseSeq.begin() + half, baseSeq.end());

            std::vector<int> result;
            result.reserve(n);
            int fi = 0, si = 0;
            bool useFirst = true;
            while (fi < static_cast<int>(first.size()) || si < static_cast<int>(second.size()))
            {
                if (useFirst && fi < static_cast<int>(first.size()))
                    result.push_back(first[fi++]);
                else if (!useFirst && si < static_cast<int>(second.size()))
                    result.push_back(second[si++]);
                else if (fi < static_cast<int>(first.size()))
                    result.push_back(first[fi++]);
                else
                    result.push_back(second[si++]);
                useFirst = !useFirst;
            }
            return result;
        }

        case ArpPath::Additive:
        {
            // Growing: 1, 1-2, 1-2-3, ..., 1-2-3-...-N
            std::vector<int> result;
            for (int len = 1; len <= n; len++)
                for (int i = 0; i < len; i++)
                    result.push_back(baseSeq[i]);
            return result;
        }

        case ArpPath::RandomLoop:
        {
            // Random permutation, with configurable loop length
            int effectiveLen = std::min(std::max(loopLen, 2), n);
            std::vector<int> pool = baseSeq;
            std::vector<int> result;
            result.reserve(effectiveLen);
            for (int i = 0; i < effectiveLen; i++)
            {
                int remaining = static_cast<int>(pool.size());
                int pick = static_cast<int>(prng() * remaining) % remaining;
                result.push_back(pool[pick]);
                pool.erase(pool.begin() + pick);
            }
            return result;
        }

        default:
            return baseSeq;
    }
}

void ArpEngine::maybeRebuildSequence(const std::vector<Zone>& zones, ArpPlayMode mode, ArpPath path, int pathLoopLen, ArpState& arp, PRNG& prng)
{
    int count = static_cast<int>(zones.size());
    int modeInt = static_cast<int>(mode);
    int pathInt = static_cast<int>(path);

    if (arp.needsRebuild || count != arp.lastZoneCount
        || modeInt != arp.lastPlayMode || pathInt != arp.lastArpPath)
    {
        auto base = buildSequence(zones, mode);
        arp.sequence = applyPath(base, path, pathLoopLen, prng);
        arp.lastZoneCount = count;
        arp.lastPlayMode = modeInt;
        arp.lastArpPath = pathInt;
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
                         static_cast<ArpPath>(persistent.arpPath),
                         persistent.arpPathLoopLen,
                         arp, transient.prng);

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
                int outMidi = zone.midi;
                float outVelocity = 0.8f;

                if (persistent.arpUseRandomization)
                {
                    auto& prng = transient.prng;

                    // Random pitch
                    if (persistent.randomPitch > 0 && prng() * 100.0f < persistent.randomPitch)
                    {
                        const auto& scaleNameStr = Scales::getAllScales()[persistent.scaleName].name;
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
                        if (shifted >= 12 && shifted <= 120) outMidi = shifted;
                    }

                    // Random velocity
                    if (persistent.randomVelocity > 0)
                    {
                        float velRange = persistent.randomVelocity / 100.0f;
                        outVelocity = outVelocity * (1.0f - velRange) + prng() * outVelocity * velRange * 2.0f;
                    }
                }

                // Apply velocity floor
                float velFloor = persistent.velocityFloor / 100.0f;
                outVelocity = std::clamp(outVelocity, velFloor, 1.0f);

                events.push_back({ outMidi, outVelocity, zoneIndex, s });

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
