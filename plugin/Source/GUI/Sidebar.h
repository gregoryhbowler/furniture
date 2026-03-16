#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Engine/State.h"
#include "../Engine/Scales.h"
#include "../Engine/Arp.h"
#include <functional>

class Sidebar : public juce::Component
{
public:
    Sidebar(juce::AudioProcessorValueTreeState& apvts,
            PersistentState& persistent,
            TransientState& transient);

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Callbacks
    std::function<void()> onResetBalls;
    std::function<void()> onClearZones;
    std::function<void()> onRandomizeNotes;
    std::function<void()> onRandomizeLayout;
    std::function<void()> onQuantize;

    // Update zone editor panel when selection changes
    void showZoneEditor(int zoneIndex);
    void hideZoneEditor();

    // Callbacks for zone editor actions
    std::function<void()> onDuplicateZone;
    std::function<void()> onDeleteZone;

private:
    juce::AudioProcessorValueTreeState& apvts;
    PersistentState& persistent;
    TransientState& transient;

    // --- Section label helper ---
    struct SectionLabel : public juce::Label
    {
        SectionLabel(const juce::String& text)
        {
            setText(text, juce::dontSendNotification);
            setFont(juce::Font(13.0f, juce::Font::bold));
            setColour(juce::Label::textColourId, juce::Colour(0xFFaaaaaa));
        }
    };

    // --- Slider + label row ---
    struct ParamRow
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

        void setup(juce::Component* parent, const juce::String& name,
                   juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
        {
            label.setText(name, juce::dontSendNotification);
            label.setFont(juce::Font(12.0f));
            label.setColour(juce::Label::textColourId, juce::Colour(0xFFcccccc));
            parent->addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
            slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF7a9966));
            slider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF555555));
            parent->addAndMakeVisible(slider);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramId, slider);
        }

        void setBounds(int x, int y, int labelW, int sliderW, int h)
        {
            label.setBounds(x, y, labelW, h);
            slider.setBounds(x + labelW, y, sliderW, h);
        }
    };

    // Physics
    SectionLabel physicsLabel{"PHYSICS"};
    ParamRow gravity, bounce, friction, speed, ballCount, ballSize, minEnergy, momentum, jitter;

    // Toggles
    juce::Label solidZonesLabel, ballCollideLabel;
    juce::ToggleButton solidZonesBtn, ballCollideBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> solidZonesAtt, ballCollideAtt;

    // Fan
    SectionLabel fanLabel{"FAN"};
    ParamRow fanAmount, fanSpeed;
    juce::Label fanDirLabel;
    juce::ComboBox fanDirBox;

    // Scale
    SectionLabel scaleLabel{"SCALE"};
    juce::Label scaleRootLabel, scaleNameLabel;
    juce::ComboBox scaleRootBox, scaleNameBox;
    juce::TextButton quantizeBtn{"Quantize"};

    // Arpeggiator
    SectionLabel arpLabel{"ARPEGGIATOR"};
    juce::Label arpEnabledLabel;
    juce::ToggleButton arpEnabledBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpEnabledAtt;

    juce::Label arpSyncLabel;
    juce::ToggleButton arpSyncBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpSyncAtt;

    ParamRow arpDivision, arpRateMs, arpRatchet;

    juce::Label arpPlayModeLabel;
    juce::ComboBox arpPlayModeBox;

    juce::Label arpPathLabel;
    juce::ComboBox arpPathBox;

    ParamRow arpPathLoopLen;

    juce::Label arpPendulumLabel;
    juce::ToggleButton arpPendulumBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpPendulumAtt;

    juce::Label arpUseRandLabel;
    juce::ToggleButton arpUseRandBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpUseRandAtt;

    // Note variation
    SectionLabel noteVarLabel{"NOTE VARIATION"};
    ParamRow randomPitch, randomOctaveChance, randomOctaveAmount, randomVelocity, velocityFloor;

    // Performance
    SectionLabel perfLabel{"PERFORMANCE"};
    ParamRow gateTime;

    // Randomize
    SectionLabel randLabel{"RANDOMIZE"};
    juce::TextButton randNotesBtn{"Randomize Notes"};
    juce::TextButton randLayoutBtn{"Randomize Layout"};

    // Actions
    SectionLabel actionsLabel{"ACTIONS"};
    juce::TextButton pauseBtn{"Pause"};
    juce::TextButton resetBallsBtn{"Reset Balls"};
    juce::TextButton clearZonesBtn{"Clear Zones"};

    // Zone editor
    SectionLabel zoneEditorLabel{"ZONE"};
    juce::Label zoneNoteLabel, zoneOctLabel;
    juce::ComboBox zoneNoteBox, zoneOctBox;
    juce::TextButton zoneDuplicateBtn{"Duplicate"};
    juce::TextButton zoneDeleteBtn{"Delete"};
    bool zoneEditorVisible = false;
    int editingZoneIndex = -1;

    void setupControls();
    void layoutRows(int startY);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sidebar)
};
