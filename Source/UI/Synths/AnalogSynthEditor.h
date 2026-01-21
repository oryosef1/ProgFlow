#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/AnalogSynth.h"
#include "../Common/WaveSelector.h"
#include "../Common/CardPanel.h"

/**
 * AnalogSynthEditor - Full UI panel for editing AnalogSynth parameters
 *
 * Saturn UI Layout with CardPanels:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ ╭─────────╮ ╭─────────╮ ╭───────────────╮                      │
 * │ │  OSC 1  │ │  OSC 2  │ │    FILTER     │                      │
 * │ │ [Wave]  │ │ [Wave]  │ │  [Type▼]      │                      │
 * │ │ ◐   ◐   │ │ ◐   ◐   │ │ ◐   ◐   ◐    │                      │
 * │ ╰─────────╯ ╰─────────╯ ╰───────────────╯                      │
 * │ ╭─────────────────────╮ ╭─────────────────────╮                │
 * │ │      AMP ENV        │ │     FILTER ENV      │                │
 * │ │  ◐   ◐   ◐   ◐     │ │  ◐   ◐   ◐   ◐     │                │
 * │ ╰─────────────────────╯ ╰─────────────────────╯                │
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
    // Card Panels (Saturn design)
    CardPanel osc1Card{"OSC 1"};
    CardPanel osc2Card{"OSC 2"};
    CardPanel filterCard{"FILTER"};
    CardPanel ampEnvCard{"AMP ENVELOPE"};
    CardPanel filterEnvCard{"FILTER ENVELOPE"};

    //==========================================================================
    // Oscillator 1
    WaveSelector osc1Wave;
    RotaryKnob osc1Octave, osc1Detune;

    // Oscillator 2
    WaveSelector osc2Wave;
    RotaryKnob osc2Octave, osc2Detune;

    //==========================================================================
    // Filter
    juce::ComboBox filterType;
    RotaryKnob filterCutoff, filterResonance, filterEnvAmount;

    //==========================================================================
    // Amp Envelope
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Filter Envelope
    RotaryKnob filterAttack, filterDecay, filterSustain, filterRelease;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "",
                   const juce::String& description = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);
    void setupWaveSelector(WaveSelector& selector, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogSynthEditor)
};
