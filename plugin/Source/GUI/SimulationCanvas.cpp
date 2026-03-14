#include "SimulationCanvas.h"
#include "../Engine/Scales.h"
#include <cmath>

SimulationCanvas::SimulationCanvas(PersistentState& p, TransientState& t, std::recursive_mutex& m)
    : persistent(p), transient(t), stateMutex(m)
{
    startTimerHz(60);
}

void SimulationCanvas::timerCallback()
{
    repaint();
}

void SimulationCanvas::paint(juce::Graphics& g)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    auto w = static_cast<float>(getWidth());
    auto h = static_cast<float>(getHeight());

    // Background
    g.fillAll(juce::Colour(0xFF2d5a3a));

    // Subtle grid
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    float gridStep = 0.1f;
    for (float gx = gridStep; gx < 1.0f; gx += gridStep)
        g.drawVerticalLine(static_cast<int>(gx * w), 0.0f, h);
    for (float gy = gridStep; gy < 1.0f; gy += gridStep)
        g.drawHorizontalLine(static_cast<int>(gy * h), 0.0f, w);

    // Draw zones
    for (size_t i = 0; i < persistent.zones.size(); i++)
    {
        auto& zone = persistent.zones[i];
        float flash = (i < transient.zoneFlash.size()) ? transient.zoneFlash[i] : 0.0f;

        auto baseColour = juce::Colour(ZONE_COLORS[zone.colorIndex % 16]);
        float alpha = 0.32f + flash * 0.5f;
        g.setColour(baseColour.withAlpha(alpha));

        float zx = (zone.cx - zone.hw) * w;
        float zy = (zone.cy - zone.hh) * h;
        float zw = zone.hw * 2.0f * w;
        float zh = zone.hh * 2.0f * h;
        g.fillRect(zx, zy, zw, zh);

        // Border
        g.setColour(baseColour.withAlpha(0.5f + flash * 0.3f));
        g.drawRect(zx, zy, zw, zh, 1.0f);

        // Label
        auto label = Scales::midiToName(zone.midi);
        g.setColour(juce::Colours::white.withAlpha(0.7f + flash * 0.3f));
        g.setFont(12.0f);
        g.drawText(label, static_cast<int>(zx), static_cast<int>(zy),
                   static_cast<int>(zw), static_cast<int>(zh),
                   juce::Justification::centred, false);

        // Selected zone: dashed outline + handles
        if (transient.selectedZone == static_cast<int>(i))
        {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            float dashes[] = { 4.0f, 4.0f };
            g.drawDashedLine(juce::Line<float>(zx, zy, zx + zw, zy), dashes, 2, 1.5f);
            g.drawDashedLine(juce::Line<float>(zx + zw, zy, zx + zw, zy + zh), dashes, 2, 1.5f);
            g.drawDashedLine(juce::Line<float>(zx + zw, zy + zh, zx, zy + zh), dashes, 2, 1.5f);
            g.drawDashedLine(juce::Line<float>(zx, zy + zh, zx, zy), dashes, 2, 1.5f);

            // Resize handles (8 small squares)
            float hs = HANDLE_SIZE * w * 0.5f;
            g.setColour(juce::Colours::white);
            auto drawHandle = [&](float hx, float hy) {
                g.fillRect(hx - hs, hy - hs, hs * 2.0f, hs * 2.0f);
            };
            drawHandle(zx, zy);               // TL
            drawHandle(zx + zw, zy);           // TR
            drawHandle(zx, zy + zh);           // BL
            drawHandle(zx + zw, zy + zh);      // BR
            drawHandle(zx + zw * 0.5f, zy);    // T
            drawHandle(zx + zw * 0.5f, zy + zh);// B
            drawHandle(zx, zy + zh * 0.5f);    // L
            drawHandle(zx + zw, zy + zh * 0.5f);// R
        }
    }

    // Draw ball trails
    float ballRadius = 0.005f + (persistent.ballSize / 18.0f) * 0.015f;

    for (auto& ball : transient.balls)
    {
        if (ball.trail.size() > 1)
        {
            for (size_t t = 1; t < ball.trail.size(); t++)
            {
                float alpha = static_cast<float>(t) / ball.trail.size() * 0.5f;
                g.setColour(juce::Colour(0xFF7dba45).withAlpha(alpha));
                g.drawLine(ball.trail[t-1].first * w, ball.trail[t-1].second * h,
                           ball.trail[t].first * w, ball.trail[t].second * h, 1.5f);
            }
        }

        // Ball
        float bx = ball.x * w;
        float by = ball.y * h;
        float br = ballRadius * w;

        juce::ColourGradient grad(juce::Colour(0xFFe8e87c), bx, by - br * 0.3f,
                                   juce::Colour(0xFF7dba45), bx, by + br, true);
        g.setGradientFill(grad);
        g.fillEllipse(bx - br, by - br, br * 2.0f, br * 2.0f);
    }
}

// --- Hit testing ---

int SimulationCanvas::hitTestZone(float nx, float ny) const
{
    // Iterate reverse (top-most zone first)
    for (int i = static_cast<int>(persistent.zones.size()) - 1; i >= 0; i--)
    {
        auto& z = persistent.zones[i];
        if (nx >= z.cx - z.hw && nx <= z.cx + z.hw &&
            ny >= z.cy - z.hh && ny <= z.cy + z.hh)
            return i;
    }
    return -1;
}

SimulationCanvas::DragMode SimulationCanvas::hitTestHandle(float nx, float ny, int idx) const
{
    if (idx < 0 || idx >= static_cast<int>(persistent.zones.size())) return None;
    auto& z = persistent.zones[idx];

    float hs = HANDLE_SIZE;
    float l = z.cx - z.hw, r = z.cx + z.hw, t = z.cy - z.hh, b = z.cy + z.hh;
    float mx = z.cx, my = z.cy;

    auto near = [&](float px, float py) { return std::abs(nx - px) < hs && std::abs(ny - py) < hs; };

    if (near(l, t)) return ResizeTL;
    if (near(r, t)) return ResizeTR;
    if (near(l, b)) return ResizeBL;
    if (near(r, b)) return ResizeBR;
    if (near(mx, t)) return ResizeT;
    if (near(mx, b)) return ResizeB;
    if (near(l, my)) return ResizeL;
    if (near(r, my)) return ResizeR;

    return None;
}

void SimulationCanvas::updateCursor(float nx, float ny)
{
    if (transient.selectedZone >= 0)
    {
        auto mode = hitTestHandle(nx, ny, transient.selectedZone);
        switch (mode)
        {
            case ResizeL: case ResizeR:
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor); return;
            case ResizeT: case ResizeB:
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor); return;
            case ResizeTL: case ResizeBR:
                setMouseCursor(juce::MouseCursor::TopLeftCornerResizeCursor); return;
            case ResizeTR: case ResizeBL:
                setMouseCursor(juce::MouseCursor::TopRightCornerResizeCursor); return;
            default: break;
        }
    }

    if (hitTestZone(nx, ny) >= 0)
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SimulationCanvas::mouseMove(const juce::MouseEvent& event)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    updateCursor(toNormX(static_cast<float>(event.x)), toNormY(static_cast<float>(event.y)));
}

void SimulationCanvas::mouseDown(const juce::MouseEvent& event)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    float nx = toNormX(static_cast<float>(event.x));
    float ny = toNormY(static_cast<float>(event.y));

    // Right-click: delete zone
    if (event.mods.isRightButtonDown())
    {
        int hit = hitTestZone(nx, ny);
        if (hit >= 0)
        {
            persistent.zones.erase(persistent.zones.begin() + hit);
            transient.zoneFlash.erase(transient.zoneFlash.begin() + hit);
            transient.zoneLockout.erase(transient.zoneLockout.begin() + hit);
            if (transient.selectedZone == hit)
            {
                transient.selectedZone = -1;
                if (onDeselectZone) onDeselectZone();
            }
            else if (transient.selectedZone > hit)
                transient.selectedZone--;
            if (onZonesChanged) onZonesChanged();
        }
        return;
    }

    // Check resize handles on selected zone first
    if (transient.selectedZone >= 0)
    {
        auto mode = hitTestHandle(nx, ny, transient.selectedZone);
        if (mode != None)
        {
            dragMode = mode;
            dragZoneIndex = transient.selectedZone;
            return;
        }
    }

    // Check zone hit
    int hit = hitTestZone(nx, ny);
    if (hit >= 0)
    {
        transient.selectedZone = hit;
        dragMode = Move;
        dragZoneIndex = hit;
        dragOffsetX = nx - persistent.zones[hit].cx;
        dragOffsetY = ny - persistent.zones[hit].cy;
        if (onSelectZone) onSelectZone(hit);
        return;
    }

    // Click on empty space: deselect or create new zone
    if (transient.selectedZone >= 0)
    {
        transient.selectedZone = -1;
        if (onDeselectZone) onDeselectZone();
    }
    else
    {
        // Create new zone
        Zone z;
        z.cx = nx;
        z.cy = ny;
        z.hw = defaultHW;
        z.hh = defaultHH;
        z.midi = Scales::nameToMidi(Scales::NOTE_NAMES[defaultNote], defaultOctave);
        z.colorIndex = StateUtils::nextColorIndex(persistent.zones);
        persistent.zones.push_back(z);
        transient.zoneFlash.push_back(0.0f);
        transient.zoneLockout.push_back(0.0f);
        transient.selectedZone = static_cast<int>(persistent.zones.size()) - 1;
        if (onSelectZone) onSelectZone(transient.selectedZone);
        if (onZonesChanged) onZonesChanged();
    }
}

void SimulationCanvas::mouseDrag(const juce::MouseEvent& event)
{
    if (dragMode == None) return;
    std::lock_guard<std::recursive_mutex> lock(stateMutex);

    float nx = toNormX(static_cast<float>(event.x));
    float ny = toNormY(static_cast<float>(event.y));

    if (dragZoneIndex < 0 || dragZoneIndex >= static_cast<int>(persistent.zones.size()))
        return;

    auto& z = persistent.zones[dragZoneIndex];
    constexpr float minHalf = 0.015f;

    switch (dragMode)
    {
        case Move:
            z.cx = std::clamp(nx - dragOffsetX, z.hw, 1.0f - z.hw);
            z.cy = std::clamp(ny - dragOffsetY, z.hh, 1.0f - z.hh);
            break;
        case ResizeL:  z.hw = std::max(minHalf, z.cx - nx); break;
        case ResizeR:  z.hw = std::max(minHalf, nx - z.cx); break;
        case ResizeT:  z.hh = std::max(minHalf, z.cy - ny); break;
        case ResizeB:  z.hh = std::max(minHalf, ny - z.cy); break;
        case ResizeTL: z.hw = std::max(minHalf, z.cx - nx); z.hh = std::max(minHalf, z.cy - ny); break;
        case ResizeTR: z.hw = std::max(minHalf, nx - z.cx); z.hh = std::max(minHalf, z.cy - ny); break;
        case ResizeBL: z.hw = std::max(minHalf, z.cx - nx); z.hh = std::max(minHalf, ny - z.cy); break;
        case ResizeBR: z.hw = std::max(minHalf, nx - z.cx); z.hh = std::max(minHalf, ny - z.cy); break;
        default: break;
    }
}

void SimulationCanvas::mouseUp(const juce::MouseEvent&)
{
    dragMode = None;
    dragZoneIndex = -1;
}

void SimulationCanvas::mouseWheelMove(const juce::MouseEvent& event,
                                       const juce::MouseWheelDetails& wheel)
{
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    float nx = toNormX(static_cast<float>(event.x));
    float ny = toNormY(static_cast<float>(event.y));

    int hit = hitTestZone(nx, ny);
    if (hit >= 0)
    {
        float factor = wheel.deltaY > 0 ? 1.08f : 0.92f;
        persistent.zones[hit].hw *= factor;
        persistent.zones[hit].hh *= factor;
        persistent.zones[hit].hw = std::max(0.015f, persistent.zones[hit].hw);
        persistent.zones[hit].hh = std::max(0.015f, persistent.zones[hit].hh);
    }
}
