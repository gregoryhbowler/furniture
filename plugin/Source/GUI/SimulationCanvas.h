#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Engine/State.h"
#include <functional>
#include <mutex>

class SimulationCanvas : public juce::Component, public juce::Timer
{
public:
    SimulationCanvas(PersistentState& persistent,
                     TransientState& transient,
                     std::recursive_mutex& stateMutex);

    void paint(juce::Graphics& g) override;
    void resized() override {}

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

    void timerCallback() override;

    // Callbacks
    std::function<void(int)> onSelectZone;
    std::function<void()> onDeselectZone;
    std::function<void()> onZonesChanged;
    std::function<void()> onResetBalls;

    int defaultNote = 0;
    int defaultOctave = 4;
    float defaultHW = 0.04f;
    float defaultHH = 0.04f;

private:
    PersistentState& persistent;
    TransientState& transient;
    std::recursive_mutex& stateMutex;

    // Drag state
    enum DragMode { None, Move, ResizeL, ResizeR, ResizeT, ResizeB,
                    ResizeTL, ResizeTR, ResizeBL, ResizeBR };
    DragMode dragMode = None;
    int dragZoneIndex = -1;
    float dragOffsetX = 0, dragOffsetY = 0;

    // Coordinate helpers
    float toNormX(float px) const { return px / static_cast<float>(getWidth()); }
    float toNormY(float py) const { return py / static_cast<float>(getHeight()); }
    float toPixX(float nx) const { return nx * static_cast<float>(getWidth()); }
    float toPixY(float ny) const { return ny * static_cast<float>(getHeight()); }

    int hitTestZone(float nx, float ny) const;
    DragMode hitTestHandle(float nx, float ny, int zoneIdx) const;
    void updateCursor(float nx, float ny);

    static constexpr float HANDLE_SIZE = 0.012f; // in normalized coords

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimulationCanvas)
};
