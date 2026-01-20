#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/ProSynth/ProSynth.h"

/**
 * ProSynthEditor - Full UI panel for editing ProSynth parameters
 *
 * Layout: Horizontal sections separated by dividers
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                                    [Master Volume]   │
 * ├─────────────────────────────────────────────────────────────────────────┤
 * │ OSC 1    │ OSC 2    │ OSC 3    │ SUB/NOISE │ FILTER   │ FLT ENV  │ AMP ENV  │ UNISON │
 * │ Mode▼    │ Mode▼    │ Mode▼    │ Sub Wave▼ │ Model▼   │ A D S R  │ A D S R  │ Voices │
 * │ Wave▼    │ Wave▼    │ Wave▼    │ NoiseType▼│ Type▼    │ Amount   │          │ Detune │
 * │ Lvl Oct  │ Lvl Oct  │ Lvl Oct  │ Sub Noise │ Cut Res  │          │          │        │
 * │ Fine     │ Fine     │ Fine     │ Lvl Lvl   │ Drive    │          │          │        │
 * └─────────────────────────────────────────────────────────────────────────┘
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

    // Section Labels
    juce::Label osc1Label, osc2Label, osc3Label, subNoiseLabel;
    juce::Label filter1Label, filterEnvLabel, ampEnvLabel, unisonLabel;

    // Divider positions for drawing
    std::vector<int> sectionDividers;

    // Master (in base class header)
    RotaryKnob glideKnob;

    //==========================================================================
    // Oscillator 1 (12.5%)
    juce::ComboBox osc1Mode, osc1Wave;
    RotaryKnob osc1Level, osc1Octave, osc1Fine;

    // Oscillator 2 (12.5%)
    juce::ComboBox osc2Mode, osc2Wave;
    RotaryKnob osc2Level, osc2Octave, osc2Fine;

    // Oscillator 3 (12.5%)
    juce::ComboBox osc3Mode, osc3Wave;
    RotaryKnob osc3Level, osc3Octave, osc3Fine;

    //==========================================================================
    // Sub Oscillator + Noise (12.5%)
    juce::ComboBox subWave, noiseType;
    RotaryKnob subLevel, noiseLevel;

    //==========================================================================
    // Filter (12.5%)
    juce::ComboBox filter1Model, filter1Type;
    RotaryKnob filterCutoff, filterResonance, filterDrive;

    //==========================================================================
    // Filter Envelope (12.5%)
    RotaryKnob filterEnvAttack, filterEnvDecay, filterEnvSustain, filterEnvRelease;
    RotaryKnob filterEnvAmount;

    //==========================================================================
    // Amp Envelope (12.5%)
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Unison (12.5%)
    RotaryKnob unisonVoices, unisonDetune;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynthEditor)
};
