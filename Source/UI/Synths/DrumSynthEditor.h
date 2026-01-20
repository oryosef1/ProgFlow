#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/DrumSynth.h"
#include <array>

/**
 * DrumSynthEditor - Full UI panel for editing DrumSynth parameters
 *
 * Layout: Horizontal sections separated by dividers
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ [Kit Dropdown]                                   [Master Volume] │
 * ├─────────────────────────────────────────────────────────────────┤
 * │ PAD GRID (4x4)         | PAD CONTROLS                            │
 * │ ┌──────┐ ┌──────┐      | ◐   ◐   ◐   ◐   ◐                    │
 * │ │ Kick │ │Snare │      | Pitch Decay Tone Level Pan             │
 * │ └──────┘ └──────┘      |                                        │
 * │ ┌──────┐ ┌──────┐      |                                        │
 * │ │ CHH  │ │ OHH  │      |                                        │
 * │ └──────┘ └──────┘      |                                        │
 * │ ... (16 pads total)    |                                        │
 * └─────────────────────────────────────────────────────────────────┘
 */
class DrumSynthEditor : public SynthEditorBase,
                        public juce::Button::Listener
{
public:
    DrumSynthEditor(DrumSynth& synth);
    ~DrumSynthEditor() override;

    void refreshFromSynth();

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    DrumSynth& synth;

    //==========================================================================
    // Section Labels
    juce::Label kitLabel;
    juce::Label padControlsLabel;

    // Divider positions for drawing
    int padGridDividerX = 0;

    //==========================================================================
    // Kit Selection (in header)
    juce::ComboBox kitSelector;

    //==========================================================================
    // Pad Grid Section (55%)
    static constexpr int NUM_PADS = 16;
    std::array<juce::TextButton, NUM_PADS> padButtons;
    int selectedPad = 0;

    //==========================================================================
    // Pad Controls Section (45%)
    RotaryKnob pitchKnob, decayKnob, toneKnob, levelKnob, panKnob;

    //==========================================================================
    // Helper methods
    void setupKnob(RotaryKnob& knob, const juce::String& label, const juce::String& suffix = "");

    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;

    void populateKits();
    void selectPad(int padIndex);
    void updatePadControls();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSynthEditor)
};
