#include "PluginProcessor.h"
#include "PluginEditor.h"

static const std::array<std::string, 5> FAN_DIR_NAMES = { "North", "South", "East", "West", "Random" };

FurnitureProcessor::FurnitureProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    persistentState = StateUtils::createDefaultPersistentState();
    transientState = StateUtils::createTransientState(persistentState);
}

FurnitureProcessor::~FurnitureProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout FurnitureProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>("gravity",    "Gravity",     0, 100, 20));
    params.push_back(std::make_unique<juce::AudioParameterInt>("bounce",     "Bounce",      50, 100, 90));
    params.push_back(std::make_unique<juce::AudioParameterInt>("friction",   "Friction",    0, 50, 4));
    params.push_back(std::make_unique<juce::AudioParameterInt>("speed",      "Speed",       5, 100, 40));
    params.push_back(std::make_unique<juce::AudioParameterInt>("ballCount",  "Balls",       0, 16, 2));
    params.push_back(std::make_unique<juce::AudioParameterInt>("ballSize",   "Ball Size",   3, 36, 7));
    params.push_back(std::make_unique<juce::AudioParameterInt>("minEnergy",  "Min Energy",  0, 80, 25));
    params.push_back(std::make_unique<juce::AudioParameterInt>("momentum",   "Momentum",    0, 100, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("jitter",     "Jitter",      0, 100, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("gateTime",   "Gate Time",   20, 2000, 250));

    params.push_back(std::make_unique<juce::AudioParameterBool>("solidZones",  "Solid Zones", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("ballCollide", "Ball Collide", false));

    params.push_back(std::make_unique<juce::AudioParameterInt>("fanAmount",    "Fan Amount",    0, 100, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("fanSpeed",     "Fan Speed",     0, 100, 30));
    params.push_back(std::make_unique<juce::AudioParameterInt>("fanDirection", "Fan Direction", 0, 4, 0));

    params.push_back(std::make_unique<juce::AudioParameterInt>("scaleRoot",    "Scale Root",  0, 11, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("scaleName",    "Scale",       0, 30, 0));

    params.push_back(std::make_unique<juce::AudioParameterInt>("randomPitch",        "Rnd Pitch",      0, 100, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("randomOctaveChance",  "Oct Chance",     0, 100, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("randomOctaveAmount",  "Oct Amount",     1, 3, 1));
    params.push_back(std::make_unique<juce::AudioParameterInt>("randomVelocity",      "Rnd Velocity",   0, 100, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("velocityFloor",       "Vel Floor",      0, 100, 10));

    // Arpeggiator
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpEnabled",   "Arp",          false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpSync",      "Arp Sync",     true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpDivision",   "Arp Division", 0, 11, 5));  // default 1/8
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpRateMs",     "Arp Rate",     10, 4000, 200));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpPlayMode",   "Arp Mode",     0, 7, 1));   // default Up
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpPath",       "Arp Path",     0, 6, 0));   // default Sequential
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpPathLoopLen","Path Loop Len",2, 16, 4));
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpPendulum",  "Arp Pendulum", false));
    params.push_back(std::make_unique<juce::AudioParameterInt>("arpRatchet",    "Arp Ratchet",  0, 16, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpUseRandomization", "Arp Note Var", false));

    return { params.begin(), params.end() };
}

void FurnitureProcessor::pullParametersFromAPVTS()
{
    persistentState.gravity      = static_cast<int>(*apvts.getRawParameterValue("gravity"));
    persistentState.bounce       = static_cast<int>(*apvts.getRawParameterValue("bounce"));
    persistentState.friction     = static_cast<int>(*apvts.getRawParameterValue("friction"));
    persistentState.speed        = static_cast<int>(*apvts.getRawParameterValue("speed"));
    persistentState.ballCount    = static_cast<int>(*apvts.getRawParameterValue("ballCount"));
    persistentState.ballSize     = static_cast<int>(*apvts.getRawParameterValue("ballSize"));
    persistentState.minEnergy    = static_cast<int>(*apvts.getRawParameterValue("minEnergy"));
    persistentState.momentum     = static_cast<int>(*apvts.getRawParameterValue("momentum"));
    persistentState.jitter       = static_cast<int>(*apvts.getRawParameterValue("jitter"));
    persistentState.gateTime     = static_cast<int>(*apvts.getRawParameterValue("gateTime"));
    persistentState.solidZones   = *apvts.getRawParameterValue("solidZones") > 0.5f;
    persistentState.ballCollide  = *apvts.getRawParameterValue("ballCollide") > 0.5f;
    persistentState.fanAmount    = static_cast<int>(*apvts.getRawParameterValue("fanAmount"));
    persistentState.fanSpeed     = static_cast<int>(*apvts.getRawParameterValue("fanSpeed"));
    persistentState.fanDirection = static_cast<int>(*apvts.getRawParameterValue("fanDirection"));
    persistentState.scaleRoot    = static_cast<int>(*apvts.getRawParameterValue("scaleRoot"));
    persistentState.scaleName    = static_cast<int>(*apvts.getRawParameterValue("scaleName"));
    persistentState.randomPitch        = static_cast<int>(*apvts.getRawParameterValue("randomPitch"));
    persistentState.randomOctaveChance = static_cast<int>(*apvts.getRawParameterValue("randomOctaveChance"));
    persistentState.randomOctaveAmount = static_cast<int>(*apvts.getRawParameterValue("randomOctaveAmount"));
    persistentState.randomVelocity     = static_cast<int>(*apvts.getRawParameterValue("randomVelocity"));
    persistentState.velocityFloor      = static_cast<int>(*apvts.getRawParameterValue("velocityFloor"));

    // Arp
    persistentState.arpEnabled   = *apvts.getRawParameterValue("arpEnabled") > 0.5f;
    persistentState.arpSync      = *apvts.getRawParameterValue("arpSync") > 0.5f;
    persistentState.arpDivision  = static_cast<int>(*apvts.getRawParameterValue("arpDivision"));
    persistentState.arpRateMs    = static_cast<int>(*apvts.getRawParameterValue("arpRateMs"));
    persistentState.arpPlayMode  = static_cast<int>(*apvts.getRawParameterValue("arpPlayMode"));
    persistentState.arpPath      = static_cast<int>(*apvts.getRawParameterValue("arpPath"));
    persistentState.arpPathLoopLen = static_cast<int>(*apvts.getRawParameterValue("arpPathLoopLen"));
    persistentState.arpPendulum  = *apvts.getRawParameterValue("arpPendulum") > 0.5f;
    persistentState.arpRatchet   = static_cast<int>(*apvts.getRawParameterValue("arpRatchet"));
    persistentState.arpUseRandomization = *apvts.getRawParameterValue("arpUseRandomization") > 0.5f;
}

void FurnitureProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    samplesPerTick = sampleRate / TICKS_PER_SECOND;
    tickAccumulator = 0.0;
}

void FurnitureProcessor::releaseResources() {}

bool FurnitureProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Accept stereo or mono output (audio is silent, but buses are needed for DAW compatibility)
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    return true;
}

void FurnitureProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear(); // no audio output

    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    pullParametersFromAPVTS();

    // Adjust ball count if parameter changed
    if (static_cast<int>(transientState.balls.size()) != persistentState.ballCount)
        transientState.balls = StateUtils::createBalls(persistentState);

    int numSamples = buffer.getNumSamples();

    // Check host transport state
    bool hostIsPlaying = true;
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        auto posInfo = playHead->getPosition();
        if (posInfo.hasValue())
        {
            if (posInfo->getBpm().hasValue())
                bpm = *posInfo->getBpm();
            hostIsPlaying = posInfo->getIsPlaying();
        }
    }

    // Process pending note-offs (always, even when stopped)
    for (auto it = pendingNoteOffs.begin(); it != pendingNoteOffs.end(); )
    {
        if (it->samplesRemaining <= numSamples)
        {
            midiMessages.addEvent(juce::MidiMessage::noteOff(1, it->midiNote, (uint8_t)0),
                                  std::min(it->samplesRemaining, numSamples - 1));
            it = pendingNoteOffs.erase(it);
        }
        else
        {
            it->samplesRemaining -= numSamples;
            ++it;
        }
    }

    // Run physics ticks at fixed rate, distributed across the buffer
    tickAccumulator += numSamples;

    while (tickAccumulator >= samplesPerTick)
    {
        tickAccumulator -= samplesPerTick;

        if (!transientState.running || !hostIsPlaying) continue;

        auto events = physics.tick(persistentState, transientState, 1.0f);

        // Convert physics events to MIDI
        int sampleOffset = static_cast<int>(numSamples - tickAccumulator);
        sampleOffset = std::clamp(sampleOffset, 0, numSamples - 1);

        for (auto& evt : events)
        {
            int midiVel = std::clamp(static_cast<int>(evt.velocity * 127.0f), 1, 127);

            midiMessages.addEvent(
                juce::MidiMessage::noteOn(1, evt.midi, static_cast<uint8_t>(midiVel)),
                sampleOffset);

            // Schedule note-off
            int gateTimeSamples = static_cast<int>(persistentState.gateTime * currentSampleRate / 1000.0);
            pendingNoteOffs.push_back({ gateTimeSamples, evt.midi });
        }
    }

    // --- Arpeggiator ---
    if (persistentState.arpEnabled && hostIsPlaying)
    {
        auto arpEvents = arp.processBlock(persistentState, transientState,
                                           numSamples, bpm, currentSampleRate);

        int gateTimeSamples = static_cast<int>(persistentState.gateTime * currentSampleRate / 1000.0);

        for (auto& evt : arpEvents)
        {
            int midiVel = std::clamp(static_cast<int>(evt.velocity * 127.0f), 1, 127);
            int offset = std::clamp(evt.sampleOffset, 0, numSamples - 1);

            midiMessages.addEvent(
                juce::MidiMessage::noteOn(1, evt.midi, static_cast<uint8_t>(midiVel)),
                offset);

            pendingNoteOffs.push_back({ gateTimeSamples, evt.midi });
        }
    }
}

void FurnitureProcessor::resetBalls()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    transientState.balls = StateUtils::createBalls(persistentState);
}

void FurnitureProcessor::syncZoneArrays()
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    transientState.zoneFlash.resize(persistentState.zones.size(), 0.0f);
    transientState.zoneLockout.resize(persistentState.zones.size(), 0.0f);
}

// --- Boilerplate ---

juce::AudioProcessorEditor* FurnitureProcessor::createEditor()
{
    return new FurnitureEditor(*this);
}

bool FurnitureProcessor::hasEditor() const { return true; }
const juce::String FurnitureProcessor::getName() const { return JucePlugin_Name; }
bool FurnitureProcessor::acceptsMidi() const { return true; }
bool FurnitureProcessor::producesMidi() const { return true; }
bool FurnitureProcessor::isMidiEffect() const { return false; }
double FurnitureProcessor::getTailLengthSeconds() const { return 0.0; }
int FurnitureProcessor::getNumPrograms() { return 1; }
int FurnitureProcessor::getCurrentProgram() { return 0; }
void FurnitureProcessor::setCurrentProgram(int) {}
const juce::String FurnitureProcessor::getProgramName(int) { return {}; }
void FurnitureProcessor::changeProgramName(int, const juce::String&) {}

void FurnitureProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Serialize persistent state + zones as JSON via ValueTree
    auto state = apvts.copyState();

    // Store zones as a child
    juce::ValueTree zonesTree("Zones");
    {
        std::lock_guard<std::recursive_mutex> lock(stateMutex);
        for (size_t i = 0; i < persistentState.zones.size(); i++)
        {
            auto& z = persistentState.zones[i];
            juce::ValueTree zt("Zone");
            zt.setProperty("cx", z.cx, nullptr);
            zt.setProperty("cy", z.cy, nullptr);
            zt.setProperty("hw", z.hw, nullptr);
            zt.setProperty("hh", z.hh, nullptr);
            zt.setProperty("midi", z.midi, nullptr);
            zt.setProperty("colorIndex", z.colorIndex, nullptr);
            zt.setProperty("placementOrder", z.placementOrder, nullptr);
            zonesTree.addChild(zt, -1, nullptr);
        }
    }
    state.addChild(zonesTree, -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FurnitureProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr) return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (state.isValid())
    {
        apvts.replaceState(state);

        // Restore zones
        auto zonesTree = state.getChildWithName("Zones");
        if (zonesTree.isValid())
        {
            std::lock_guard<std::recursive_mutex> lock(stateMutex);
            persistentState.zones.clear();
            for (int i = 0; i < zonesTree.getNumChildren(); i++)
            {
                auto zt = zonesTree.getChild(i);
                Zone z;
                z.cx = zt.getProperty("cx");
                z.cy = zt.getProperty("cy");
                z.hw = zt.getProperty("hw");
                z.hh = zt.getProperty("hh");
                z.midi = zt.getProperty("midi");
                z.colorIndex = zt.getProperty("colorIndex");
                z.placementOrder = zt.getProperty("placementOrder", static_cast<int>(i));
                persistentState.zones.push_back(z);
            }
            transientState = StateUtils::createTransientState(persistentState);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FurnitureProcessor();
}
