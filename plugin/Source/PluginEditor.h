#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/SimulationCanvas.h"
#include "GUI/Sidebar.h"

class FurnitureEditor : public juce::AudioProcessorEditor
{
public:
    explicit FurnitureEditor(FurnitureProcessor& processor);
    ~FurnitureEditor() override;

    void resized() override;

private:
    FurnitureProcessor& processor;

    SimulationCanvas canvas;
    juce::Viewport sidebarViewport;
    Sidebar sidebar;

    static constexpr int SIDEBAR_WIDTH = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FurnitureEditor)
};
