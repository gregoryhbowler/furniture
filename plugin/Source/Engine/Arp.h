#pragma once

#include "State.h"
#include <vector>
#include <string>
#include <array>

// Note divisions as fractions of a whole note (4 beats)
enum class ArpDivision
{
    Div1_64,   // 1/16 of a beat
    Div1_32,   // 1/8 of a beat
    Div1_32T,  // triplet
    Div1_16,
    Div1_16T,
    Div1_8,
    Div1_8T,
    Div1_4,
    Div1_4T,
    Div1_2,
    Div1_2T,
    Div1_1,    // whole note (1 bar in 4/4)
    COUNT
};

enum class ArpPlayMode
{
    FirstPlaced,
    Up,
    Down,
    Random,
    LeftRight,
    RightLeft,
    VerticalUp,
    VerticalDown,
    COUNT
};

inline const char* arpDivisionName(ArpDivision d)
{
    static const char* names[] = {
        "1/64", "1/32", "1/32T", "1/16", "1/16T",
        "1/8", "1/8T", "1/4", "1/4T", "1/2", "1/2T", "1 Bar"
    };
    return names[static_cast<int>(d)];
}

inline const char* arpPlayModeName(ArpPlayMode m)
{
    static const char* names[] = {
        "First Placed", "Up", "Down", "Random",
        "Left-Right", "Right-Left", "Vertical Up", "Vertical Down"
    };
    return names[static_cast<int>(m)];
}

// Returns the division value in beats (quarter notes)
inline float divisionToBeats(ArpDivision d)
{
    static const float values[] = {
        1.0f/16, 1.0f/8, 1.0f/12, 1.0f/4, 1.0f/6,
        1.0f/2, 1.0f/3, 1.0f, 2.0f/3, 2.0f, 4.0f/3, 4.0f
    };
    return values[static_cast<int>(d)];
}

// Convert division + BPM to milliseconds
inline float divisionToMs(ArpDivision division, float bpm)
{
    float beats = divisionToBeats(division);
    float msPerBeat = 60000.0f / bpm;
    return beats * msPerBeat;
}

// Convert division + BPM to samples
inline double divisionToSamples(ArpDivision division, double bpm, double sampleRate)
{
    double beats = static_cast<double>(divisionToBeats(division));
    double samplesPerBeat = sampleRate * 60.0 / bpm;
    return beats * samplesPerBeat;
}

class ArpEngine
{
public:
    // Build sorted sequence of zone indices based on play mode
    static std::vector<int> buildSequence(const std::vector<Zone>& zones, ArpPlayMode mode);

    // Run arp for a block of samples. Returns events with sample offsets.
    // bpm comes from host transport (or manual setting).
    struct ArpEvent
    {
        int midi;
        float velocity;
        int zoneIndex;
        int sampleOffset;  // position within the buffer
    };

    std::vector<ArpEvent> processBlock(PersistentState& persistent,
                                        TransientState& transient,
                                        int numSamples,
                                        double bpm,
                                        double sampleRate);

private:
    void maybeRebuildSequence(const std::vector<Zone>& zones, ArpPlayMode mode, ArpState& arp);
    void advancePosition(ArpState& arp, ArpPlayMode mode, bool pendulum, PRNG& prng);
};
