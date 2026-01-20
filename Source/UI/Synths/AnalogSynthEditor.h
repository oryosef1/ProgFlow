#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/AnalogSynth.h"
#include "../Common/WaveSelector.h"

/**
 * AnalogSynthEditor - Full UI panel for editing AnalogSynth parameters
 *
 * Layout: Horizontal sections separated by dividers
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ OSC 1 | OSC 2 | FILTER      | AMP ENV       | FLT ENV          │
 * │ Wave▼ | Wave▼ | Type▼       |               |                  │
 * │ ◐  ◐  | ◐  ◐  | ◐   ◐   ◐  | ◐  ◐  ◐  ◐  | ◐  ◐  ◐  ◐     │
 * │Semi Fn|Semi Dt|Cut Res  Env | A  D  S  R   | A  D  S  R       │
 * └─────────────────────────────────────────────────────────────────┘
 */
class AnalogSynthEditor : public SynthEditorBase
{
public:
    AnalogSynthEditor(AnalogSynth& synth);
    ~AnalogSynthEditor() override;

    // Refresh UI from synth parameters (e.g., after preset load)
    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    AnalogSynth& synth;

    //==========================================================================
    // Section Labels
    juce::Label osc1Label, osc2Label;
    juce::Label filterLabel, ampEnvLabel, filterEnvLabel;

    // Divider positions for drawing
    std::vector<int> sectionDividers;

    //==========================================================================
    // Oscillator 1 (15%)
    WaveSelector osc1Wave;
    RotaryKnob osc1Octave, osc1Detune;

    // Oscillator 2 (15%)
    WaveSelector osc2Wave;
    RotaryKnob osc2Octave, osc2Detune;

    //==========================================================================
    // Filter (20%)
    juce::ComboBox filterType;
    RotaryKnob filterCutoff, filterResonance, filterEnvAmount;

    //==========================================================================
    // Amp Envelope (25%)
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Filter Envelope (25%)
    RotaryKnob filterAttack, filterDecay, filterSustain, filterRelease;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);
    void setupWaveSelector(WaveSelector& selector, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogSynthEditor)
};
