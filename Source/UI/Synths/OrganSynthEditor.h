#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/OrganSynth.h"

/**
 * OrganSynthEditor - Full UI panel for editing OrganSynth parameters
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │                        DRAWBARS                                  │
 * │   16'   5⅓'   8'    4'   2⅔'   2'   1⅗'  1⅓'   1'              │
 * │   [0]   [1]   [2]   [3]  [4]   [5]  [6]  [7]   [8]              │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ PERCUSSION   │ ROTARY        │ DRIVE        │ KEY CLICK         │
 * │ Type  Harm   │ Speed  Depth  │ [Knob]       │ [Knob]            │
 * │ Decay        │               │              │                   │
 * └─────────────────────────────────────────────────────────────────┘
 */
class OrganSynthEditor : public SynthEditorBase
{
public:
    OrganSynthEditor(OrganSynth& synth);
    ~OrganSynthEditor() override;

    // Refresh UI from synth parameters (e.g., after preset load)
    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    OrganSynth& synth;

    //==========================================================================
    // Section Labels
    juce::Label drawbarsLabel;
    juce::Label percussionLabel;
    juce::Label rotaryLabel;
    juce::Label driveLabel;
    juce::Label keyClickLabel;

    // Divider positions for drawing
    int row1Bottom = 0;  // Horizontal divider after drawbars row
    std::vector<int> row2Dividers;  // Vertical dividers in row 2

    //==========================================================================

    // Drawbars (9 knobs in a row)
    std::array<RotaryKnob, 9> drawbars;
    std::array<juce::Label, 9> drawbarLabels;

    //==========================================================================
    // Percussion (3 combo boxes)
    juce::ComboBox percussionType;
    juce::ComboBox percussionDecay;
    juce::ComboBox percussionHarmonic;

    //==========================================================================
    // Rotary Speaker (1 combo + 1 knob)
    juce::ComboBox rotarySpeed;
    RotaryKnob rotaryDepth;

    //==========================================================================
    // Drive (1 knob)
    RotaryKnob driveKnob;

    //==========================================================================
    // Key Click (1 knob)
    RotaryKnob keyClickKnob;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganSynthEditor)
};
