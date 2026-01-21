#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/Sampler.h"
#include "../Common/CardPanel.h"

/**
 * SamplerEditor - Full UI panel for editing Sampler parameters
 *
 * Saturn UI Layout with CardPanels (2 rows):
 * Row 1: PLAYBACK | FILTER | AMP ENV
 * Row 2: FILTER ENV | SAMPLE INFO
 */
class SamplerEditor : public SynthEditorBase,
                      public juce::Button::Listener
{
public:
    SamplerEditor(Sampler& synth);
    ~SamplerEditor() override;

    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    Sampler& synth;

    //==========================================================================
    // Card Panels (Saturn design)
    CardPanel playbackCard{"PLAYBACK"};
    CardPanel filterCard{"FILTER"};
    CardPanel ampEnvCard{"AMP ENV"};
    CardPanel filterEnvCard{"FILTER ENV"};
    CardPanel sampleInfoCard{"SAMPLE"};

    //==========================================================================
    // Playback
    RotaryKnob transposeKnob, fineTuneKnob, startKnob;
    juce::ComboBox loopModeSelector;

    //==========================================================================
    // Filter
    RotaryKnob filterCutoff, filterResonance, filterEnvAmount;

    //==========================================================================
    // Amp Envelope
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Filter Envelope
    RotaryKnob filterAttack, filterDecay, filterSustain, filterRelease;

    //==========================================================================
    // Sample info
    juce::Label sampleInfoText;
    juce::TextButton loadSampleButton;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "",
                   const juce::String& description = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);

    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;
    void populatePresets();
    void updateSampleInfo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerEditor)
};
