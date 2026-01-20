#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * WaveSelector - A visual wave type selector with icon buttons
 *
 * Displays waveform icons (sine, triangle, sawtooth, square) as clickable buttons.
 * Looks much nicer than a dropdown ComboBox.
 */
class WaveSelector : public juce::Component
{
public:
    WaveSelector();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Set/get selected wave index (0=sine, 1=triangle, 2=sawtooth, 3=square)
    void setSelectedIndex(int index, juce::NotificationType notification = juce::sendNotification);
    int getSelectedIndex() const { return selectedIndex; }

    // Callback when selection changes
    std::function<void(int)> onSelectionChanged;

    // Option to make it smaller
    void setCompact(bool compact) { isCompact = compact; repaint(); }

private:
    int selectedIndex = 0;
    bool isCompact = false;

    // Wave type icons (Unicode characters that look like waveforms)
    static constexpr const char* waveIcons[] = {
        "\xe2\x88\xbf",      // ∿ Sine wave
        "\xe2\x96\xb3",      // △ Triangle
        "\xe2\x8a\xbf",      // ⊿ Sawtooth (right triangle)
        "\xe2\x8a\x93"       // ⊓ Square
    };

    static constexpr const char* waveNames[] = {
        "Sine", "Triangle", "Sawtooth", "Square"
    };

    int getIndexAtPosition(int x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveSelector)
};
