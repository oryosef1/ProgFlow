#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/ProSynth/ProSynth.h"
#include "../Common/CardPanel.h"

/**
 * ProSynthEditor - Full UI panel for editing ProSynth parameters
 *
 * Saturn UI Layout with CardPanels (2 rows):
 * Row 1: OSC1 | OSC2 | OSC3 | SUB+NOISE | FILTER
 * Row 2: FILTER ENV | AMP ENV | UNISON
 */
class ProSynthEditor : public SynthEditorBase
{
public:
    ProSynthEditor(ProSynth& synth);
    ~ProSynthEditor() override;

    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    ProSynth& synth;

    //==========================================================================
    // Card Panels (Saturn design)
    CardPanel osc1Card{"OSC 1"};
    CardPanel osc2Card{"OSC 2"};
    CardPanel osc3Card{"OSC 3"};
    CardPanel subNoiseCard{"SUB + NOISE"};
    CardPanel filterCard{"FILTER"};
    CardPanel filterEnvCard{"FILTER ENV"};
    CardPanel ampEnvCard{"AMP ENV"};
    CardPanel unisonCard{"UNISON"};

    //==========================================================================
    // Oscillator 1
    juce::ComboBox osc1Mode, osc1Wave;
    RotaryKnob osc1Level, osc1Octave, osc1Fine;

    // Oscillator 2
    juce::ComboBox osc2Mode, osc2Wave;
    RotaryKnob osc2Level, osc2Octave, osc2Fine;

    // Oscillator 3
    juce::ComboBox osc3Mode, osc3Wave;
    RotaryKnob osc3Level, osc3Octave, osc3Fine;

    //==========================================================================
    // Sub Oscillator + Noise
    juce::ComboBox subWave, noiseType;
    RotaryKnob subLevel, noiseLevel;

    //==========================================================================
    // Filter
    juce::ComboBox filter1Model, filter1Type;
    RotaryKnob filterCutoff, filterResonance, filterDrive;

    //==========================================================================
    // Filter Envelope
    RotaryKnob filterEnvAttack, filterEnvDecay, filterEnvSustain, filterEnvRelease;
    RotaryKnob filterEnvAmount;

    //==========================================================================
    // Amp Envelope
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Unison
    RotaryKnob unisonVoices, unisonDetune;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "",
                   const juce::String& description = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    void comboBoxChanged(juce::ComboBox* box) override;
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynthEditor)
};
