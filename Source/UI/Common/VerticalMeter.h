#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * VerticalMeter - A vertical VU meter component
 *
 * Displays the audio level with:
 * - Green: -∞ to -12 dB
 * - Yellow: -12 to -6 dB
 * - Red: -6 to 0 dB
 *
 * Uses peak hold and smooth falloff for visual appeal.
 */
class VerticalMeter : public juce::Component,
                       public juce::Timer
{
public:
    VerticalMeter();
    ~VerticalMeter() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    /**
     * Set the current level (0.0 to 1.0+)
     * Values above 1.0 indicate clipping
     */
    void setLevel(float level);

    /**
     * Set whether to show the peak indicator
     */
    void setShowPeak(bool show) { showPeak = show; }

    /**
     * Set the decay speed (0.0 = instant, 1.0 = very slow)
     */
    void setDecaySpeed(float speed) { decaySpeed = juce::jlimit(0.0f, 0.99f, speed); }

private:
    float currentLevel = 0.0f;
    float displayLevel = 0.0f;
    float peakLevel = 0.0f;
    int peakHoldCounter = 0;

    bool showPeak = true;
    float decaySpeed = 0.85f;

    static constexpr int peakHoldTime = 30;  // frames to hold peak
    static constexpr float peakDecay = 0.95f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalMeter)
};
