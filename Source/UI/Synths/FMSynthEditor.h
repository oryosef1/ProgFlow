#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/FMSynth.h"
#include "../Common/CardPanel.h"

/**
 * FMSynthEditor - Full UI panel for editing FMSynth parameters
 *
 * Saturn UI Layout with CardPanels:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  ╭───────────╮ ╭─────────╮ ╭───────────╮ ╭───────────╮         │
 * │  │ ALGORITHM │ │ CARRIER │ │MODULATOR 1│ │MODULATOR 2│         │
 * │  │ [▼ Serial]│ │ [Ratio] │ │[Rat][Idx] │ │[Rat][Idx] │         │
 * │  ╰───────────╯ ╰─────────╯ ╰───────────╯ ╰───────────╯         │
 * │  ╭─────────╮ ╭─────────────────────────────────────────────────╮│
 * │  │FEEDBACK │ │                  ENVELOPES                      ││
 * │  │ [Knob]  │ │  AMP [ADSR]    MOD 1 [ADSR]    MOD 2 [ADSR]    ││
 * │  ╰─────────╯ ╰─────────────────────────────────────────────────╯│
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
    // Card Panels (Saturn design)
    CardPanel algorithmCard{"ALGORITHM"};
    CardPanel carrierCard{"CARRIER"};
    CardPanel mod1Card{"MODULATOR 1"};
    CardPanel mod2Card{"MODULATOR 2"};
    CardPanel feedbackCard{"FEEDBACK"};
    CardPanel envelopesCard{"ENVELOPES"};

    // Envelope sub-labels (inside envelopes card)
    juce::Label ampEnvLabel, mod1EnvLabel, mod2EnvLabel;

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
                   const juce::String& label, const juce::String& suffix = "",
                   const juce::String& description = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynthEditor)
};
