#include "Scales.h"
#include <set>
#include <cmath>
#include <algorithm>

namespace Scales
{

static std::vector<ScaleDef> buildScaleTable()
{
    return {
        // Major modes
        { "Major (Ionian)",    {0,2,4,5,7,9,11} },
        { "Dorian",            {0,2,3,5,7,9,10} },
        { "Phrygian",          {0,1,3,5,7,8,10} },
        { "Lydian",            {0,2,4,6,7,9,11} },
        { "Mixolydian",        {0,2,4,5,7,9,10} },
        { "Minor (Aeolian)",   {0,2,3,5,7,8,10} },
        { "Locrian",           {0,1,3,5,6,8,10} },
        // Melodic / harmonic
        { "Harmonic Minor",    {0,2,3,5,7,8,11} },
        { "Melodic Minor",     {0,2,3,5,7,9,11} },
        { "Harmonic Major",    {0,2,4,5,7,8,11} },
        // Pentatonics & blues
        { "Pentatonic Major",  {0,2,4,7,9} },
        { "Pentatonic Minor",  {0,3,5,7,10} },
        { "Blues",              {0,3,5,6,7,10} },
        { "Blues Major",        {0,2,3,4,7,9} },
        // Symmetric
        { "Whole Tone",        {0,2,4,6,8,10} },
        { "Diminished (HW)",   {0,1,3,4,6,7,9,10} },
        { "Diminished (WH)",   {0,2,3,5,6,8,9,11} },
        { "Augmented",         {0,3,4,7,8,11} },
        // Exotic
        { "Hungarian Minor",   {0,2,3,6,7,8,11} },
        { "Phrygian Dominant", {0,1,4,5,7,8,10} },
        { "Double Harmonic",   {0,1,4,5,7,8,11} },
        { "Hirajoshi",         {0,2,3,7,8} },
        { "In Sen",            {0,1,5,7,10} },
        { "Yo",                {0,2,5,7,9} },
        { "Iwato",             {0,1,5,6,10} },
        { "Kumoi",             {0,2,3,7,9} },
        { "Bebop Dominant",    {0,2,4,5,7,9,10,11} },
        { "Bebop Major",       {0,2,4,5,7,8,9,11} },
        { "Prometheus",        {0,2,4,6,9,10} },
        { "Enigmatic",         {0,1,4,6,8,10,11} },
        // Chromatic
        { "Chromatic",         {0,1,2,3,4,5,6,7,8,9,10,11} },
    };
}

const std::vector<ScaleDef>& getAllScales()
{
    static auto scales = buildScaleTable();
    return scales;
}

int getScaleIndex(const std::string& name)
{
    auto& scales = getAllScales();
    for (int i = 0; i < static_cast<int>(scales.size()); i++)
        if (scales[i].name == name)
            return i;
    return 0;
}

static const std::vector<int>& getIntervals(const std::string& scaleName)
{
    auto& scales = getAllScales();
    for (auto& s : scales)
        if (s.name == scaleName)
            return s.intervals;
    return scales[0].intervals; // default Major
}

std::string midiToName(int midi)
{
    int note = midi % 12;
    int octave = midi / 12 - 1;
    return NOTE_NAMES[note] + std::to_string(octave);
}

int nameToMidi(const std::string& name, int octave)
{
    for (int i = 0; i < 12; i++)
        if (NOTE_NAMES[i] == name)
            return (octave + 1) * 12 + i;
    return 60;
}

float midiToFrequency(int midi)
{
    return 440.0f * std::pow(2.0f, (midi - 69) / 12.0f);
}

std::vector<int> getScaleNotes(int rootIdx, const std::string& scaleName, int octLow, int octHigh)
{
    auto& intervals = getIntervals(scaleName);
    std::vector<int> notes;
    for (int oct = octLow; oct <= octHigh; oct++)
    {
        for (auto iv : intervals)
        {
            int midi = (oct + 1) * 12 + rootIdx + iv;
            if (midi >= 0 && midi <= 127)
                notes.push_back(midi);
        }
    }
    return notes;
}

std::vector<int> getScaleNotesInOctave(int midi, int rootIdx, const std::string& scaleName)
{
    auto& intervals = getIntervals(scaleName);
    int octave = midi / 12;
    std::vector<int> notes;
    for (auto iv : intervals)
    {
        int note = octave * 12 + ((rootIdx + iv) % 12);
        if (note >= 0 && note <= 127)
            notes.push_back(note);
    }
    return notes;
}

int quantizeToScale(int midi, int rootIdx, const std::string& scaleName)
{
    auto& intervals = getIntervals(scaleName);
    std::set<int> validPCs;
    for (auto iv : intervals)
        validPCs.insert((rootIdx + iv) % 12);

    if (validPCs.count(midi % 12))
        return midi;

    for (int offset = 1; offset <= 6; offset++)
    {
        int up = midi + offset;
        if (up <= 127 && validPCs.count(up % 12))
            return up;
        int down = midi - offset;
        if (down >= 0 && validPCs.count(down % 12))
            return down;
    }
    return midi;
}

} // namespace Scales
