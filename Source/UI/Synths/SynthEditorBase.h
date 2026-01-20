#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"
#include "../Common/RotaryKnob.h"

/**
 * SynthEditorBase - Base class for all synth editors
 * Provides shared header layout and divider drawing utilities
 */
class SynthEditorBase : public juce::Component,
                        public juce::ComboBox::Listener
{
public:
    SynthEditorBase();
    ~SynthEditorBase() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

protected:
    static constexpr int HEADER_HEIGHT = 72;  // Increased to fit knob (48) + label (16) + padding
    static constexpr int SECTION_PADDING = 12;
    static constexpr int KNOB_SIZE = 48;

    // Header components
    juce::Label presetLabel;
    juce::ComboBox presetSelector;
    juce::Label masterLabel;
    RotaryKnob masterVolume;

    // Divider drawing
    void drawVerticalDivider(juce::Graphics& g, int x, int yStart, int yEnd);
    void drawHorizontalDivider(juce::Graphics& g, int xStart, int xEnd, int y);

    // Section box drawing (with rounded corners and title)
    void drawSectionBox(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title = "");

    // Section label helper
    void createSectionLabel(juce::Label& label, const juce::String& text);

    // Get content area below header
    juce::Rectangle<int> getContentArea();

    // Subclasses must implement
    virtual void layoutContent(juce::Rectangle<int> area) = 0;
    virtual void drawDividers(juce::Graphics& g, juce::Rectangle<int> area);

private:
    void layoutHeader(juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthEditorBase)
};
