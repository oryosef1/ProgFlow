#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/MarkerTrack.h"
#include <functional>

/**
 * TimeRuler - Bar/beat markers at top of timeline
 *
 * Displays bar numbers, beat divisions, and markers.
 * Clicking seeks to that position. Double-click adds a marker.
 */
class TimeRuler : public juce::Component
{
public:
    TimeRuler();

    void setBarWidth(int width);
    int getBarWidth() const { return barWidth; }

    void setScrollOffset(double offset);  // In bars
    double getScrollOffset() const { return scrollOffset; }

    void setTimeSignature(int numerator, int denominator);

    // Loop region display
    void setLoopRegion(double startBar, double endBar);
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled; }

    // Marker support
    void setMarkerTrack(MarkerTrack* track);
    void setBpm(double bpm);

    void paint(juce::Graphics& g) override;

    // Click to seek, double-click to add marker
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Callback with bar position when seeking
    std::function<void(double)> onSeek;

    // Callback when marker is added (beat position)
    std::function<void(double)> onMarkerAdd;

private:
    int barWidth = 100;
    double scrollOffset = 0.0;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;

    // Loop region
    bool loopEnabled = false;
    double loopStartBar = 0.0;
    double loopEndBar = 4.0;

    // Markers
    MarkerTrack* markerTrack = nullptr;
    double currentBpm = 120.0;

    double xToBar(int x) const;
    double xToBeat(int x) const;
    void drawLoopRegion(juce::Graphics& g);
    void drawMarkers(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeRuler)
};
