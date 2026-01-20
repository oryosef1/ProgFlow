#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/AudioClip.h"
#include "WaveformComponent.h"
#include <functional>

/**
 * AudioClipComponent - Visual representation of an AudioClip on the timeline
 *
 * Shows a colored rectangle with the clip name and waveform preview.
 * Supports drag to move, edge drag to resize/trim, and fade handles.
 */
class AudioClipComponent : public juce::Component
{
public:
    AudioClipComponent(AudioClip& clip, double bpm, int pixelsPerBeat, int trackHeight);

    void setBpm(double newBpm);
    double getBpm() const { return bpm; }

    void setPixelsPerBeat(int ppb);
    int getPixelsPerBeat() const { return pixelsPerBeat; }

    void setTrackHeight(int height);
    int getTrackHeight() const { return trackHeight; }

    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    void updateFromClip();  // Sync visual position from AudioClip data

    AudioClip& getClip() { return clip; }
    const AudioClip& getClip() const { return clip; }

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(AudioClipComponent*)> onSelected;
    std::function<void(AudioClipComponent*)> onDoubleClicked;
    std::function<void(double)> onMoved;           // New start beat
    std::function<void(double, double)> onTrimmed; // New trim start, trim end
    std::function<void(AudioClipComponent*)> onDeleted;

private:
    AudioClip& clip;
    double bpm;
    int pixelsPerBeat;
    int trackHeight;
    bool selected = false;

    // Waveform display
    WaveformComponent waveformDisplay;

    // Drag state
    enum class DragMode { None, Move, TrimLeft, TrimRight, FadeIn, FadeOut };
    DragMode dragMode = DragMode::None;
    double dragStartBeat = 0.0;
    juce::int64 dragStartTrimStart = 0;
    juce::int64 dragStartTrimEnd = 0;

    // Visual constants
    static constexpr int TRIM_HANDLE_WIDTH = 8;
    static constexpr int FADE_HANDLE_SIZE = 12;
    static constexpr int HEADER_HEIGHT = 18;

    // Helper methods
    DragMode getDragModeForPosition(int x, int y) const;
    void updateCursor(int x, int y);
    void drawFadeOverlays(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClipComponent)
};
