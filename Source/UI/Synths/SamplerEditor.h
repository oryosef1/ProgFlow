#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Synths/Sampler.h"
#include "../Common/RotaryKnob.h"
#include "../LookAndFeel.h"

/**
 * SamplerEditor - Full UI panel for editing Sampler parameters
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ PLAYBACK        │ FILTER              │ AMP ENV                 │
 * │ Transpose Fine  │ Cutoff  Res  EnvAmt │ A  D  S  R              │
 * │ Start  Loop     │                     │                         │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ FILTER ENV                            │ SAMPLE INFO             │
 * │ A  D  S  R                            │ (Zone display)          │
 * └─────────────────────────────────────────────────────────────────┘
 */
class SamplerEditor : public juce::Component,
                       public juce::ComboBox::Listener,
                       public juce::Button::Listener
{
public:
    SamplerEditor(Sampler& synth);
    ~SamplerEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshFromSynth();

private:
    Sampler& synth;

    // Section Labels
    juce::Label presetLabel;
    juce::Label masterLabel;
    juce::Label playbackLabel;
    juce::Label filterLabel;
    juce::Label ampEnvLabel;
    juce::Label filterEnvLabel;
    juce::Label sampleInfoLabel;

    // Preset Section
    juce::ComboBox presetSelector;

    // Master Volume
    RotaryKnob masterVolume;

    // Playback
    RotaryKnob transposeKnob, fineTuneKnob, startKnob;
    juce::ComboBox loopModeSelector;

    // Filter
    RotaryKnob filterCutoff, filterResonance, filterEnvAmount;

    // Amp Envelope
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    // Filter Envelope
    RotaryKnob filterAttack, filterDecay, filterSustain, filterRelease;

    // Sample info display
    juce::Label sampleInfoText;
    juce::TextButton loadSampleButton;

    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);
    void createSectionLabel(juce::Label& label, const juce::String& text);

    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;
    void populatePresets();
    void updateSampleInfo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerEditor)
};
