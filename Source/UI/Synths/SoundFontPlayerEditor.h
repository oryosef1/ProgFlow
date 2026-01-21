#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/SoundFontPlayer.h"
#include "../Common/CardPanel.h"

/**
 * SoundFontPlayerEditor - Full UI panel for GM SoundFont instrument selection
 *
 * Saturn UI Layout with CardPanels:
 * Row 1: CONTROLS | ENVELOPE | SOUNDFONT INFO
 *
 * Features:
 * - Browse 128 GM instruments by category
 * - Load custom SoundFont files
 * - Adjust volume, pan, envelope overrides
 */
class SoundFontPlayerEditor : public SynthEditorBase,
                               public juce::Button::Listener
{
public:
    SoundFontPlayerEditor(SoundFontPlayer& synth);
    ~SoundFontPlayerEditor() override;

    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    SoundFontPlayer& synth;

    //==========================================================================
    // Card Panels (Saturn design)
    CardPanel controlsCard{"CONTROLS"};
    CardPanel envelopeCard{"ENVELOPE"};
    CardPanel soundFontCard{"SOUNDFONT"};

    //==========================================================================
    // Instrument Selection (in header)
    juce::ComboBox categorySelector;
    juce::ComboBox instrumentSelector;

    //==========================================================================
    // Controls
    RotaryKnob volumeKnob, panKnob;
    RotaryKnob pitchBendKnob, modWheelKnob;

    //==========================================================================
    // Envelope Overrides
    RotaryKnob attackKnob, releaseKnob;

    //==========================================================================
    // SoundFont Info
    juce::TextButton loadSF2Button;
    juce::Label soundFontPath;

    //==========================================================================
    // GM Categories
    static const int NUM_CATEGORIES = 16;
    static const char* categoryNames[NUM_CATEGORIES];

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "",
                   const juce::String& description = "");

    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;

    void populateCategories();
    void populateInstruments(int categoryIndex);
    void updateSoundFontInfo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontPlayerEditor)
};
