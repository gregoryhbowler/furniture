#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/PhysicsEngine.h"
#include "Engine/State.h"
#include <mutex>

class FurnitureProcessor : public juce::AudioProcessor
{
public:
    FurnitureProcessor();
    ~FurnitureProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Shared state access (thread-safe via mutex)
    PersistentState& getPersistentState() { return persistentState; }
    TransientState& getTransientState() { return transientState; }
    std::recursive_mutex& getStateMutex() { return stateMutex; }

    // Called by editor when ball count/speed changes
    void resetBalls();

    // Called by editor to sync zone flash/lockout arrays after zone add/remove
    void syncZoneArrays();

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Sync APVTS parameter values into persistentState
    void pullParametersFromAPVTS();

    PhysicsEngine physics;
    PersistentState persistentState;
    TransientState transientState;
    std::recursive_mutex stateMutex;

    double currentSampleRate = 48000.0;
    double samplesPerTick = 0.0;
    double tickAccumulator = 0.0;

    // Pending note-offs: {sampleOffset, midiNote}
    struct PendingNoteOff
    {
        int samplesRemaining;
        int midiNote;
    };
    std::vector<PendingNoteOff> pendingNoteOffs;

    static constexpr int TICKS_PER_SECOND = 180; // 60fps * 3 ticks/frame

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FurnitureProcessor)
};
