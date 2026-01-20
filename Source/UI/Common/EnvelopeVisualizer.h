#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * EnvelopeVisualizer - Mini ADSR curve display
 *
 * Features:
 * - Visualizes Attack, Decay, Sustain, Release envelope
 * - Real-time updates as knob values change
 * - Compact size (~100x50px)
 * - Filled curve with accent color
 */
class EnvelopeVisualizer : public juce::Component
{
public:
    EnvelopeVisualizer();
    ~EnvelopeVisualizer() override = default;

    //==========================================================================
    // Set envelope values (0.0 - 1.0 normalized)
    void setAttack(float value);
    void setDecay(float value);
    void setSustain(float value);
    void setRelease(float value);

    // Set all at once
    void setADSR(float attack, float decay, float sustain, float release);

    //==========================================================================
    // Appearance
    void setAccentColour(juce::Colour colour);
    void setBackgroundColour(juce::Colour colour);

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;

    //==========================================================================
    // Static constants
    static constexpr int DEFAULT_WIDTH = 100;
    static constexpr int DEFAULT_HEIGHT = 50;

private:
    // Normalized values (0-1)
    float attack = 0.1f;
    float decay = 0.3f;
    float sustain = 0.7f;
    float release = 0.4f;

    // Colours
    juce::Colour accentColour;
    juce::Colour backgroundColour;
    bool useCustomAccent = false;
    bool useCustomBg = false;

    // Build the envelope path
    juce::Path buildEnvelopePath(juce::Rectangle<float> bounds) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeVisualizer)
};
