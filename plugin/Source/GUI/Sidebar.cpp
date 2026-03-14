#include "Sidebar.h"

Sidebar::Sidebar(juce::AudioProcessorValueTreeState& a, PersistentState& p, TransientState& t)
    : apvts(a), persistent(p), transient(t)
{
    setupControls();
}

void Sidebar::setupControls()
{
    // Physics section
    addAndMakeVisible(physicsLabel);
    gravity.setup(this,   "Gravity",    apvts, "gravity");
    bounce.setup(this,    "Bounce",     apvts, "bounce");
    friction.setup(this,  "Friction",   apvts, "friction");
    speed.setup(this,     "Speed",      apvts, "speed");
    ballCount.setup(this, "Balls",      apvts, "ballCount");
    ballSize.setup(this,  "Ball Size",  apvts, "ballSize");
    minEnergy.setup(this, "Min Energy", apvts, "minEnergy");
    momentum.setup(this,  "Momentum",   apvts, "momentum");
    jitter.setup(this,    "Jitter",     apvts, "jitter");

    // Toggles
    solidZonesLabel.setText("Solid Zones", juce::dontSendNotification);
    solidZonesLabel.setFont(juce::Font(12.0f));
    solidZonesLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(solidZonesLabel);
    addAndMakeVisible(solidZonesBtn);
    solidZonesAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "solidZones", solidZonesBtn);

    ballCollideLabel.setText("Ball Collide", juce::dontSendNotification);
    ballCollideLabel.setFont(juce::Font(12.0f));
    ballCollideLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(ballCollideLabel);
    addAndMakeVisible(ballCollideBtn);
    ballCollideAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "ballCollide", ballCollideBtn);

    // Fan section
    addAndMakeVisible(fanLabel);
    fanAmount.setup(this, "Amount", apvts, "fanAmount");
    fanSpeed.setup(this,  "Speed",  apvts, "fanSpeed");

    fanDirLabel.setText("Direction", juce::dontSendNotification);
    fanDirLabel.setFont(juce::Font(12.0f));
    fanDirLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(fanDirLabel);

    fanDirBox.addItem("North",  1);
    fanDirBox.addItem("South",  2);
    fanDirBox.addItem("East",   3);
    fanDirBox.addItem("West",   4);
    fanDirBox.addItem("Random", 5);
    fanDirBox.setSelectedId(persistent.fanDirection + 1);
    fanDirBox.onChange = [this]() {
        if (auto* param = apvts.getParameter("fanDirection"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(fanDirBox.getSelectedId() - 1)));
    };
    addAndMakeVisible(fanDirBox);

    // Arpeggiator section
    addAndMakeVisible(arpLabel);

    arpEnabledLabel.setText("Arp", juce::dontSendNotification);
    arpEnabledLabel.setFont(juce::Font(12.0f));
    arpEnabledLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(arpEnabledLabel);
    addAndMakeVisible(arpEnabledBtn);
    arpEnabledAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "arpEnabled", arpEnabledBtn);

    arpSyncLabel.setText("Sync", juce::dontSendNotification);
    arpSyncLabel.setFont(juce::Font(12.0f));
    arpSyncLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(arpSyncLabel);
    addAndMakeVisible(arpSyncBtn);
    arpSyncAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "arpSync", arpSyncBtn);

    arpDivision.setup(this, "Division", apvts, "arpDivision");
    arpRateMs.setup(this, "Rate (ms)", apvts, "arpRateMs");

    arpPlayModeLabel.setText("Mode", juce::dontSendNotification);
    arpPlayModeLabel.setFont(juce::Font(12.0f));
    arpPlayModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(arpPlayModeLabel);

    for (int i = 0; i < static_cast<int>(ArpPlayMode::COUNT); i++)
        arpPlayModeBox.addItem(arpPlayModeName(static_cast<ArpPlayMode>(i)), i + 1);
    arpPlayModeBox.setSelectedId(persistent.arpPlayMode + 1);
    arpPlayModeBox.onChange = [this]() {
        if (auto* param = apvts.getParameter("arpPlayMode"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(arpPlayModeBox.getSelectedId() - 1)));
    };
    addAndMakeVisible(arpPlayModeBox);

    arpPendulumLabel.setText("Pendulum", juce::dontSendNotification);
    arpPendulumLabel.setFont(juce::Font(12.0f));
    arpPendulumLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(arpPendulumLabel);
    addAndMakeVisible(arpPendulumBtn);
    arpPendulumAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "arpPendulum", arpPendulumBtn);

    arpRatchet.setup(this, "Ratchet", apvts, "arpRatchet");

    arpUseRandLabel.setText("Note Var", juce::dontSendNotification);
    arpUseRandLabel.setFont(juce::Font(12.0f));
    arpUseRandLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(arpUseRandLabel);
    addAndMakeVisible(arpUseRandBtn);
    arpUseRandAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, "arpUseRandomization", arpUseRandBtn);

    // Scale section
    addAndMakeVisible(scaleLabel);

    scaleRootLabel.setText("Root", juce::dontSendNotification);
    scaleRootLabel.setFont(juce::Font(12.0f));
    scaleRootLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(scaleRootLabel);

    for (int i = 0; i < 12; i++)
        scaleRootBox.addItem(Scales::NOTE_NAMES[i], i + 1);
    scaleRootBox.setSelectedId(persistent.scaleRoot + 1);
    scaleRootBox.onChange = [this]() {
        if (auto* param = apvts.getParameter("scaleRoot"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(scaleRootBox.getSelectedId() - 1)));
    };
    addAndMakeVisible(scaleRootBox);

    scaleNameLabel.setText("Scale", juce::dontSendNotification);
    scaleNameLabel.setFont(juce::Font(12.0f));
    scaleNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addAndMakeVisible(scaleNameLabel);

    auto& scales = Scales::getAllScales();
    for (int i = 0; i < static_cast<int>(scales.size()); i++)
        scaleNameBox.addItem(scales[i].name, i + 1);
    scaleNameBox.setSelectedId(persistent.scaleName + 1);
    scaleNameBox.onChange = [this]() {
        if (auto* param = apvts.getParameter("scaleName"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(scaleNameBox.getSelectedId() - 1)));
    };
    addAndMakeVisible(scaleNameBox);

    addAndMakeVisible(quantizeBtn);
    quantizeBtn.onClick = [this]() { if (onQuantize) onQuantize(); };

    // Note variation
    addAndMakeVisible(noteVarLabel);
    randomPitch.setup(this,        "Rnd Pitch",  apvts, "randomPitch");
    randomOctaveChance.setup(this, "Oct Chance",  apvts, "randomOctaveChance");
    randomOctaveAmount.setup(this, "Oct Amount",  apvts, "randomOctaveAmount");
    randomVelocity.setup(this,     "Rnd Velocity",apvts, "randomVelocity");
    velocityFloor.setup(this,      "Vel Floor",   apvts, "velocityFloor");

    // Performance
    addAndMakeVisible(perfLabel);
    gateTime.setup(this, "Gate Time", apvts, "gateTime");

    // Randomize
    addAndMakeVisible(randLabel);
    addAndMakeVisible(randNotesBtn);
    randNotesBtn.onClick = [this]() { if (onRandomizeNotes) onRandomizeNotes(); };
    addAndMakeVisible(randLayoutBtn);
    randLayoutBtn.onClick = [this]() { if (onRandomizeLayout) onRandomizeLayout(); };

    // Actions
    addAndMakeVisible(actionsLabel);
    addAndMakeVisible(pauseBtn);
    pauseBtn.onClick = [this]() {
        transient.running = !transient.running;
        pauseBtn.setButtonText(transient.running ? "Pause" : "Run");
    };
    addAndMakeVisible(resetBallsBtn);
    resetBallsBtn.onClick = [this]() { if (onResetBalls) onResetBalls(); };
    addAndMakeVisible(clearZonesBtn);
    clearZonesBtn.onClick = [this]() { if (onClearZones) onClearZones(); };

    // Zone editor (initially hidden)
    addChildComponent(zoneEditorLabel);
    zoneNoteLabel.setText("Note", juce::dontSendNotification);
    zoneNoteLabel.setFont(juce::Font(12.0f));
    zoneNoteLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addChildComponent(zoneNoteLabel);
    for (int i = 0; i < 12; i++)
        zoneNoteBox.addItem(Scales::NOTE_NAMES[i], i + 1);
    zoneNoteBox.onChange = [this]() {
        if (editingZoneIndex >= 0 && editingZoneIndex < static_cast<int>(persistent.zones.size()))
        {
            int noteIdx = zoneNoteBox.getSelectedId() - 1;
            int octave = persistent.zones[editingZoneIndex].midi / 12 - 1;
            persistent.zones[editingZoneIndex].midi = Scales::nameToMidi(Scales::NOTE_NAMES[noteIdx], octave);
        }
    };
    addChildComponent(zoneNoteBox);

    zoneOctLabel.setText("Octave", juce::dontSendNotification);
    zoneOctLabel.setFont(juce::Font(12.0f));
    zoneOctLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
    addChildComponent(zoneOctLabel);
    for (int i = 0; i <= 8; i++)
        zoneOctBox.addItem(juce::String(i), i + 1);
    zoneOctBox.onChange = [this]() {
        if (editingZoneIndex >= 0 && editingZoneIndex < static_cast<int>(persistent.zones.size()))
        {
            int noteIdx = persistent.zones[editingZoneIndex].midi % 12;
            int octave = zoneOctBox.getSelectedId() - 1;
            persistent.zones[editingZoneIndex].midi = (octave + 1) * 12 + noteIdx;
        }
    };
    addChildComponent(zoneOctBox);

    addChildComponent(zoneDuplicateBtn);
    zoneDuplicateBtn.onClick = [this]() { if (onDuplicateZone) onDuplicateZone(); };
    addChildComponent(zoneDeleteBtn);
    zoneDeleteBtn.onClick = [this]() { if (onDeleteZone) onDeleteZone(); };
}

void Sidebar::showZoneEditor(int zoneIndex)
{
    editingZoneIndex = zoneIndex;
    zoneEditorVisible = true;

    if (zoneIndex >= 0 && zoneIndex < static_cast<int>(persistent.zones.size()))
    {
        int midi = persistent.zones[zoneIndex].midi;
        zoneNoteBox.setSelectedId((midi % 12) + 1, juce::dontSendNotification);
        zoneOctBox.setSelectedId((midi / 12 - 1) + 1, juce::dontSendNotification);
    }

    zoneEditorLabel.setVisible(true);
    zoneNoteLabel.setVisible(true);
    zoneNoteBox.setVisible(true);
    zoneOctLabel.setVisible(true);
    zoneOctBox.setVisible(true);
    zoneDuplicateBtn.setVisible(true);
    zoneDeleteBtn.setVisible(true);
    resized();
}

void Sidebar::hideZoneEditor()
{
    zoneEditorVisible = false;
    editingZoneIndex = -1;
    zoneEditorLabel.setVisible(false);
    zoneNoteLabel.setVisible(false);
    zoneNoteBox.setVisible(false);
    zoneOctLabel.setVisible(false);
    zoneOctBox.setVisible(false);
    zoneDuplicateBtn.setVisible(false);
    zoneDeleteBtn.setVisible(false);
    resized();
}

void Sidebar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1a1a1a));
}

void Sidebar::resized()
{
    int w = getWidth();
    int labelW = 75;
    int sliderW = w - labelW - 10;
    int rowH = 22;
    int pad = 2;
    int x = 5;
    int y = 5;

    auto nextRow = [&]() { y += rowH + pad; };
    auto placeSection = [&](SectionLabel& lbl) { lbl.setBounds(x, y, w - 10, rowH); nextRow(); };
    auto placeRow = [&](ParamRow& r) { r.setBounds(x, y, labelW, sliderW, rowH); nextRow(); };
    auto placeToggle = [&](juce::Label& lbl, juce::ToggleButton& btn) {
        lbl.setBounds(x, y, labelW, rowH);
        btn.setBounds(x + labelW, y, 30, rowH);
        nextRow();
    };
    auto placeCombo = [&](juce::Label& lbl, juce::ComboBox& box) {
        lbl.setBounds(x, y, labelW, rowH);
        box.setBounds(x + labelW, y, sliderW, rowH);
        nextRow();
    };
    auto placeButton = [&](juce::TextButton& btn) {
        btn.setBounds(x, y, w - 10, 24);
        y += 26 + pad;
    };

    // Physics
    placeSection(physicsLabel);
    placeRow(gravity);
    placeRow(bounce);
    placeRow(friction);
    placeRow(speed);
    placeRow(ballCount);
    placeRow(ballSize);
    placeRow(minEnergy);
    placeRow(momentum);
    placeRow(jitter);
    placeToggle(solidZonesLabel, solidZonesBtn);
    placeToggle(ballCollideLabel, ballCollideBtn);

    y += 4;

    // Fan
    placeSection(fanLabel);
    placeRow(fanAmount);
    placeRow(fanSpeed);
    placeCombo(fanDirLabel, fanDirBox);

    y += 4;

    // Arpeggiator
    placeSection(arpLabel);
    placeToggle(arpEnabledLabel, arpEnabledBtn);
    placeToggle(arpSyncLabel, arpSyncBtn);
    placeRow(arpDivision);
    placeRow(arpRateMs);
    placeCombo(arpPlayModeLabel, arpPlayModeBox);
    placeToggle(arpPendulumLabel, arpPendulumBtn);
    placeRow(arpRatchet);
    placeToggle(arpUseRandLabel, arpUseRandBtn);

    y += 4;

    // Scale
    placeSection(scaleLabel);
    placeCombo(scaleRootLabel, scaleRootBox);
    placeCombo(scaleNameLabel, scaleNameBox);
    placeButton(quantizeBtn);

    y += 4;

    // Note variation
    placeSection(noteVarLabel);
    placeRow(randomPitch);
    placeRow(randomOctaveChance);
    placeRow(randomOctaveAmount);
    placeRow(randomVelocity);
    placeRow(velocityFloor);

    y += 4;

    // Performance
    placeSection(perfLabel);
    placeRow(gateTime);

    y += 4;

    // Randomize
    placeSection(randLabel);
    placeButton(randNotesBtn);
    placeButton(randLayoutBtn);

    y += 4;

    // Actions
    placeSection(actionsLabel);
    placeButton(pauseBtn);
    placeButton(resetBallsBtn);
    placeButton(clearZonesBtn);

    // Zone editor
    if (zoneEditorVisible)
    {
        y += 8;
        placeSection(zoneEditorLabel);
        placeCombo(zoneNoteLabel, zoneNoteBox);
        placeCombo(zoneOctLabel, zoneOctBox);
        placeButton(zoneDuplicateBtn);
        placeButton(zoneDeleteBtn);
    }
}
