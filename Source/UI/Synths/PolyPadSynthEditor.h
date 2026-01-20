#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/PolyPadSynth.h"

/**
 * PolyPadSynthEditor - Full UI panel for editing PolyPadSynth parameters
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  OSCILLATORS          │  FILTER                │  CHORUS        │
 * │  Wave1▼  Wave2▼       │  Type▼                 │  Rate Depth   │
 * │  Detune  Mix          │  Cutoff Reso EnvAmt   │  Mix           │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  AMP ENVELOPE                │  FILTER ENVELOPE                 │
 * │  A    D    S    R            │  A    D    S    R                │
 * └─────────────────────────────────────────────────────────────────┘
 */
class PolyPadSynthEditor : public SynthEditorBase
{
public:
    PolyPadSynthEditor(PolyPadSynth& synth);
    ~PolyPadSynthEditor() override;

    // Refresh UI from synth parameters (e.g., after preset load)
    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    PolyPadSynth& synth;

    //==========================================================================
    // Section Labels
    juce::Label oscLabel, filterLabel, chorusLabel;
    juce::Label ampEnvLabel, filterEnvLabel;

    // Divider positions for drawing
    std::vector<int> sectionDividers;
    int rowDividerY;

    //==========================================================================
    // Oscillators
    juce::ComboBox osc1Wave, osc2Wave;
    RotaryKnob osc2Detune, oscMix;

    //==========================================================================
    // Filter
    juce::ComboBox filterType;
    RotaryKnob filterCutoff, filterResonance, filterEnvAmount;

    //==========================================================================
    // Chorus
    RotaryKnob chorusRate, chorusDepth, chorusMix;

    //==========================================================================
    // Amp Envelope
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Filter Envelope
    RotaryKnob filterAttack, filterDecay, filterSustain, filterRelease;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyPadSynthEditor)
};
