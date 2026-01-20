#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Synths/SoundFontPlayer.h"
#include "../Common/RotaryKnob.h"
#include "../LookAndFeel.h"

/**
 * SoundFontPlayerEditor - Full UI panel for GM SoundFont instrument selection
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Category Dropdown] [Instrument Dropdown]        [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ CONTROLS           │ ENVELOPE              │ SoundFont Info      │
 * │ Volume  Pan        │ Attack  Release       │ [Load SF2...]       │
 * │ PitchBend ModWheel │                       │ (path display)      │
 * └─────────────────────────────────────────────────────────────────┘
 *
 * Features:
 * - Browse 128 GM instruments by category
 * - Load custom SoundFont files
 * - Adjust volume, pan, envelope overrides
 */
class SoundFontPlayerEditor : public juce::Component,
                               public juce::ComboBox::Listener,
                               public juce::Button::Listener
{
public:
    SoundFontPlayerEditor(SoundFontPlayer& synth);
    ~SoundFontPlayerEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshFromSynth();

private:
    SoundFontPlayer& synth;

    // Section Labels
    juce::Label categoryLabel;
    juce::Label instrumentLabel;
    juce::Label masterLabel;
    juce::Label controlsLabel;
    juce::Label envelopeLabel;
    juce::Label soundFontLabel;

    // Instrument Selection
    juce::ComboBox categorySelector;
    juce::ComboBox instrumentSelector;

    // Master Volume
    RotaryKnob masterVolume;

    // Controls
    RotaryKnob volumeKnob, panKnob;
    RotaryKnob pitchBendKnob, modWheelKnob;

    // Envelope Overrides
    RotaryKnob attackKnob, releaseKnob;

    // SoundFont Info
    juce::TextButton loadSF2Button;
    juce::Label soundFontPath;

    // GM Categories
    static const int NUM_CATEGORIES = 16;
    static const char* categoryNames[NUM_CATEGORIES];

    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void createSectionLabel(juce::Label& label, const juce::String& text);

    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;

    void populateCategories();
    void populateInstruments(int categoryIndex);
    void updateSoundFontInfo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontPlayerEditor)
};
