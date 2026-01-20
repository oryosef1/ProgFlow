#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/StringSynth.h"

/**
 * StringSynthEditor - Full UI panel for editing StringSynth parameters
 *
 * Layout: Two rows with horizontal sections separated by dividers
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Preset Dropdown]                               [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ SECTIONS        │ ENSEMBLE   │ FILTER          │ AMP ENV       │
 * │ Violins Violas  │ Voices     │ Cutoff  Res     │ A  D  S  R    │
 * │ Cellos  Basses  │ Spread     │ EnvAmt          │               │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ FILTER ENV      │ CHORUS                │ PHASER               │
 * │ A  D  S  R      │ Rate  Depth  Mix      │ Mix                  │
 * └─────────────────────────────────────────────────────────────────┘
 */
class StringSynthEditor : public SynthEditorBase
{
public:
    StringSynthEditor(StringSynth& synth);
    ~StringSynthEditor() override;

    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    StringSynth& synth;

    // Section Labels
    juce::Label sectionsLabel;
    juce::Label ensembleLabel;
    juce::Label filterLabel;
    juce::Label ampEnvLabel;
    juce::Label filterEnvLabel;
    juce::Label chorusLabel;
    juce::Label phaserLabel;

    // Divider positions for drawing
    std::vector<int> row1Dividers;
    std::vector<int> row2Dividers;
    int rowDividerY;

    //==========================================================================
    // String Sections (25%)
    RotaryKnob violinsKnob, violasKnob, cellosKnob, bassesKnob;

    // Ensemble (25%)
    RotaryKnob ensembleVoices, ensembleSpread;

    // Filter (25%)
    RotaryKnob filterCutoff, filterResonance, filterEnvAmount;

    // Amp Envelope (25%)
    RotaryKnob ampAttack, ampDecay, ampSustain, ampRelease;

    //==========================================================================
    // Filter Envelope (33%)
    RotaryKnob filterAttack, filterDecay, filterSustain, filterRelease;

    // Chorus (33%)
    RotaryKnob chorusRate, chorusDepth, chorusWet;

    // Phaser (33%)
    RotaryKnob phaserWet;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");

    // ComboBox::Listener
    void comboBoxChanged(juce::ComboBox* box) override;

    // Populate preset list
    void populatePresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSynthEditor)
};
