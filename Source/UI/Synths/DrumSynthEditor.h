#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/DrumSynth.h"
#include "../Common/CardPanel.h"
#include <array>

/**
 * DrumSynthEditor - Full UI panel for editing DrumSynth parameters
 *
 * Saturn UI Layout with CardPanels:
 * Row 1: PAD GRID card | PAD CONTROLS card
 */
class DrumSynthEditor : public SynthEditorBase,
                        public juce::Button::Listener,
                        public juce::Timer
{
public:
    DrumSynthEditor(DrumSynth& synth);
    ~DrumSynthEditor() override;

    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;
    void timerCallback() override;

private:
    DrumSynth& synth;

    //==========================================================================
    // Card Panels (Saturn design)
    CardPanel padGridCard{"PAD GRID"};
    CardPanel padControlsCard{"PAD CONTROLS"};

    //==========================================================================
    // Kit Selection (in header)
    juce::ComboBox kitSelector;

    //==========================================================================
    // Pad Grid Section
    static constexpr int NUM_PADS = 16;
    std::array<juce::TextButton, NUM_PADS> padButtons;
    int selectedPad = 0;
    int flashingPad = -1;  // Pad currently flashing (-1 = none)

    //==========================================================================
    // Pad Controls Section
    RotaryKnob pitchKnob, decayKnob, toneKnob, levelKnob, panKnob;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& label,
                   const juce::String& suffix = "", const juce::String& description = "");

    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;

    void populateKits();
    void selectPad(int padIndex);
    void flashPad(int padIndex);
    void updatePadAppearance();
    void updatePadControls();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSynthEditor)
};
