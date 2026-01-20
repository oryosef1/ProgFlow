#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/MidiClip.h"
#include <functional>

/**
 * ClipComponent - Visual representation of a MidiClip on the timeline
 *
 * Shows a colored rectangle with the clip name and a mini preview of notes.
 * Supports drag to move, edge drag to resize, and double-click to edit.
 */
class ClipComponent : public juce::Component
{
public:
    ClipComponent(MidiClip& clip, int barWidth, int trackHeight);

    void setBarWidth(int width);
    int getBarWidth() const { return barWidth; }

    void setTrackHeight(int height);
    int getTrackHeight() const { return trackHeight; }

    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    void updateFromClip();  // Sync visual position from MidiClip data

    MidiClip& getClip() { return clip; }
    const MidiClip& getClip() const { return clip; }

    void paint(juce::Graphics& g) override;

    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(ClipComponent*)> onSelected;
    std::function<void(ClipComponent*)> onDoubleClicked;
    std::function<void(double, double)> onMoved;      // New start bar, new duration
    std::function<void(double)> onResized;            // New duration
    std::function<void(ClipComponent*)> onDeleted;

private:
    MidiClip& clip;
    int barWidth;
    int trackHeight;
    bool selected = false;

    // Drag state
    enum class DragMode { None, Move, ResizeLeft, ResizeRight };
    DragMode dragMode = DragMode::None;
    double dragStartBar = 0.0;
    double dragStartDuration = 0.0;

    // Visual constants
    static constexpr int RESIZE_HANDLE_WIDTH = 8;
    static constexpr int HEADER_HEIGHT = 20;

    // Helper methods
    DragMode getDragModeForPosition(int x) const;
    void drawNotePreviews(juce::Graphics& g, juce::Rectangle<int> bounds);
    void updateCursor(int x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
