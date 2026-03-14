#include "PluginEditor.h"
#include "Engine/Scales.h"

FurnitureEditor::FurnitureEditor(FurnitureProcessor& p)
    : AudioProcessorEditor(p),
      processor(p),
      canvas(p.getPersistentState(), p.getTransientState(), p.getStateMutex()),
      sidebar(p.getAPVTS(), p.getPersistentState(), p.getTransientState())
{
    setSize(1000, 650);
    setResizable(true, true);
    setResizeLimits(700, 400, 1920, 1200);

    addAndMakeVisible(canvas);

    sidebarViewport.setViewedComponent(&sidebar, false);
    sidebarViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(sidebarViewport);

    // Wire up callbacks
    canvas.onSelectZone = [this](int idx) {
        sidebar.showZoneEditor(idx);
    };

    canvas.onDeselectZone = [this]() {
        sidebar.hideZoneEditor();
    };

    canvas.onZonesChanged = [this]() {
        processor.syncZoneArrays();
    };

    sidebar.onResetBalls = [this]() {
        processor.resetBalls();
    };

    sidebar.onClearZones = [this]() {
        std::lock_guard<std::recursive_mutex> lock(processor.getStateMutex());
        processor.getPersistentState().zones.clear();
        processor.getTransientState().zoneFlash.clear();
        processor.getTransientState().zoneLockout.clear();
        processor.getTransientState().selectedZone = -1;
        sidebar.hideZoneEditor();
    };

    sidebar.onQuantize = [this]() {
        std::lock_guard<std::recursive_mutex> lock(processor.getStateMutex());
        auto& ps = processor.getPersistentState();
        auto& scales = Scales::getAllScales();
        std::string scaleName = (ps.scaleName < static_cast<int>(scales.size()))
                                    ? scales[ps.scaleName].name : "Major (Ionian)";
        for (auto& z : ps.zones)
            z.midi = Scales::quantizeToScale(z.midi, ps.scaleRoot, scaleName);
    };

    sidebar.onRandomizeNotes = [this]() {
        std::lock_guard<std::recursive_mutex> lock(processor.getStateMutex());
        auto& ps = processor.getPersistentState();
        auto& scales = Scales::getAllScales();
        std::string scaleName = (ps.scaleName < static_cast<int>(scales.size()))
                                    ? scales[ps.scaleName].name : "Major (Ionian)";
        auto notes = Scales::getScaleNotes(ps.scaleRoot, scaleName, 2, 6);
        if (notes.empty()) return;
        for (auto& z : ps.zones)
            z.midi = notes[std::rand() % notes.size()];
    };

    sidebar.onRandomizeLayout = [this]() {
        std::lock_guard<std::recursive_mutex> lock(processor.getStateMutex());
        for (auto& z : processor.getPersistentState().zones)
        {
            z.hw = 0.02f + (std::rand() / static_cast<float>(RAND_MAX)) * 0.06f;
            z.hh = 0.02f + (std::rand() / static_cast<float>(RAND_MAX)) * 0.06f;
            z.cx = z.hw + (std::rand() / static_cast<float>(RAND_MAX)) * (1.0f - 2.0f * z.hw);
            z.cy = z.hh + (std::rand() / static_cast<float>(RAND_MAX)) * (1.0f - 2.0f * z.hh);
        }
    };

    sidebar.onDuplicateZone = [this]() {
        std::lock_guard<std::recursive_mutex> lock(processor.getStateMutex());
        auto& ps = processor.getPersistentState();
        auto& ts = processor.getTransientState();
        if (ts.selectedZone >= 0 && ts.selectedZone < static_cast<int>(ps.zones.size()))
        {
            Zone dup = ps.zones[ts.selectedZone];
            dup.cx = std::min(dup.cx + 0.05f, 1.0f - dup.hw);
            dup.cy = std::min(dup.cy + 0.05f, 1.0f - dup.hh);
            dup.colorIndex = StateUtils::nextColorIndex(ps.zones);
            ps.zones.push_back(dup);
            ts.zoneFlash.push_back(0.0f);
            ts.zoneLockout.push_back(0.0f);
            ts.selectedZone = static_cast<int>(ps.zones.size()) - 1;
            sidebar.showZoneEditor(ts.selectedZone);
        }
    };

    sidebar.onDeleteZone = [this]() {
        std::lock_guard<std::recursive_mutex> lock(processor.getStateMutex());
        auto& ps = processor.getPersistentState();
        auto& ts = processor.getTransientState();
        if (ts.selectedZone >= 0 && ts.selectedZone < static_cast<int>(ps.zones.size()))
        {
            ps.zones.erase(ps.zones.begin() + ts.selectedZone);
            ts.zoneFlash.erase(ts.zoneFlash.begin() + ts.selectedZone);
            ts.zoneLockout.erase(ts.zoneLockout.begin() + ts.selectedZone);
            ts.selectedZone = -1;
            sidebar.hideZoneEditor();
        }
    };

    // Set sidebar height for scrolling
    sidebar.setSize(SIDEBAR_WIDTH - 12, 1200);
}

FurnitureEditor::~FurnitureEditor() {}

void FurnitureEditor::resized()
{
    auto area = getLocalBounds();
    auto sidebarArea = area.removeFromRight(SIDEBAR_WIDTH);

    canvas.setBounds(area);
    sidebarViewport.setBounds(sidebarArea);
    sidebar.setSize(SIDEBAR_WIDTH - 12, sidebar.getHeight());
}
