#pragma once

#include <vector>
#include <string>
#include <array>
#include <cmath>

namespace Scales
{
    // 12 note names
    inline const std::array<std::string, 12> NOTE_NAMES = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    // Scale definitions: name + interval pattern
    struct ScaleDef
    {
        std::string name;
        std::vector<int> intervals;
    };

    const std::vector<ScaleDef>& getAllScales();
    int getScaleIndex(const std::string& name);

    // MIDI utilities
    std::string midiToName(int midi);
    int nameToMidi(const std::string& name, int octave);
    float midiToFrequency(int midi);

    // Scale note queries
    std::vector<int> getScaleNotes(int rootIdx, const std::string& scaleName, int octLow, int octHigh);
    std::vector<int> getScaleNotesInOctave(int midi, int rootIdx, const std::string& scaleName);
    int quantizeToScale(int midi, int rootIdx, const std::string& scaleName);
}
