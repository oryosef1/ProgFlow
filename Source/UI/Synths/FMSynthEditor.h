#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/FMSynth.h"

/**
 * FMSynthEditor - Full UI panel for editing FMSynth parameters
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  ALGORITHM  │  CARRIER   │  MODULATOR 1  │  MODULATOR 2  │ FEEDBACK │
 * │  [Dropdown] │  Ratio     │  Ratio  Index │  Ratio  Index │  [Knob]  │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  AMP ENV        │  MOD 1 ENV      │  MOD 2 ENV                  │
 * │  A  D  S  R     │  A  D  S  R     │  A  D  S  R                 │
 * └─────────────────────────────────────────────────────────────────┘
 */
class FMSynthEditor : public SynthEditorBase
{
public:
    FMSynthEditor(FMSynth& synth);
    ~FMSynthEditor() override;

    // Refresh UI from synth parameters (e.g., after preset load)
    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    FMSynth& synth;

    //==========================================================================
    // Section Labels
    juce::Label algorithmLabel;
    juce::Label carrierLabel, mod1Label, mod2Label;
    juce::Label feedbackLabel, ampEnvLabel, mod1EnvLabel, mod2EnvLabel;

    // Divider positions for drawing
    std::vector<int> sectionDividers;
    int rowDividerY;

    //==========================================================================
    // Algorithm
    juce::ComboBox algorithmSelector;

    //==========================================================================
    // Carrier
    RotaryKnob carrierRatio;

    //==========================================================================
    // Modulator 1
    RotaryKnob mod1Ratio, mod1Index;

    //==========================================================================
    // Modulator 2
    RotaryKnob mod2Ratio, mod2Index;

    //==========================================================================
    // Feedback
    RotaryKnob feedbackKnob;

    //==========================================================================
    // Amp Envelope
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Mod 1 Envelope
    RotaryKnob mod1Attack, mod1Decay, mod1Sustain, mod1Release;

    //==========================================================================
    // Mod 2 Envelope
    RotaryKnob mod2Attack, mod2Decay, mod2Sustain, mod2Release;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynthEditor)
};
