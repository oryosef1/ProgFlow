#include "DrumSynthEditor.h"

DrumSynthEditor::DrumSynthEditor(DrumSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Kit Section (in header, next to presets)
    createSectionLabel(kitLabel, "KIT");
    addAndMakeVisible(kitLabel);

    kitSelector.addListener(this);
    addAndMakeVisible(kitSelector);
    populateKits();

    // Master Volume (inherited from base)
    masterVolume.setLabel("Volume");
    if (auto* param = synth.getParameterInfo("volume"))
    {
        masterVolume.setRange(param->minValue, param->maxValue, param->step);
        masterVolume.setDefaultValue(param->defaultValue);
        masterVolume.setValue(param->value, juce::dontSendNotification);
    }
    masterVolume.onValueChange = [this](float value)
    {
        synth.setParameter("volume", value);
    };

    // Pad Buttons (4x4 grid)
    for (int i = 0; i < NUM_PADS; ++i)
    {
        padButtons[i].setButtonText(synth.getPadName(i));
        padButtons[i].addListener(this);
        padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        addAndMakeVisible(padButtons[i]);
    }

    // Pad Controls Section
    createSectionLabel(padControlsLabel, "PAD CONTROLS");
    addAndMakeVisible(padControlsLabel);

    setupKnob(pitchKnob, "Pitch");
    setupKnob(decayKnob, "Decay");
    setupKnob(toneKnob, "Tone");
    setupKnob(levelKnob, "Level");
    setupKnob(panKnob, "Pan");

    // Set up knob callbacks
    pitchKnob.onValueChange = [this](float value)
    {
        synth.setPadParameter(selectedPad, "pitch", value);
    };
    decayKnob.onValueChange = [this](float value)
    {
        synth.setPadParameter(selectedPad, "decay", value);
    };
    toneKnob.onValueChange = [this](float value)
    {
        synth.setPadParameter(selectedPad, "tone", value);
    };
    levelKnob.onValueChange = [this](float value)
    {
        synth.setPadParameter(selectedPad, "level", value);
    };
    panKnob.onValueChange = [this](float value)
    {
        synth.setPadParameter(selectedPad, "pan", value);
    };

    // Select first pad
    selectPad(0);
    refreshFromSynth();
}

DrumSynthEditor::~DrumSynthEditor()
{
    kitSelector.removeListener(this);
    for (auto& btn : padButtons)
        btn.removeListener(this);
}

void DrumSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& label, const juce::String& suffix)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);
    knob.setRange(0.0f, 1.0f, 0.01f);
    knob.setDefaultValue(0.5f);
    addAndMakeVisible(knob);
}


void DrumSynthEditor::populateKits()
{
    kitSelector.clear();

    auto kits = synth.getAvailableKits();
    int id = 1;
    for (const auto& kit : kits)
    {
        kitSelector.addItem(kit, id++);
    }

    // Select current kit
    juce::String currentKit = synth.getCurrentKit();
    int index = kits.indexOf(currentKit);
    if (index >= 0)
        kitSelector.setSelectedId(index + 1, juce::dontSendNotification);
    else
        kitSelector.setSelectedId(1, juce::dontSendNotification);
}

void DrumSynthEditor::selectPad(int padIndex)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    // Update button colors
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (i == padIndex)
            padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
        else
            padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    }

    selectedPad = padIndex;
    updatePadControls();
}

void DrumSynthEditor::updatePadControls()
{
    // Set knob ranges
    pitchKnob.setRange(0.5f, 2.0f, 0.01f);
    pitchKnob.setDefaultValue(1.0f);
    panKnob.setRange(-1.0f, 1.0f, 0.01f);
    panKnob.setDefaultValue(0.0f);

    // Get current values
    pitchKnob.setValue(synth.getPadParameter(selectedPad, "pitch"), juce::dontSendNotification);
    decayKnob.setValue(synth.getPadParameter(selectedPad, "decay"), juce::dontSendNotification);
    toneKnob.setValue(synth.getPadParameter(selectedPad, "tone"), juce::dontSendNotification);
    levelKnob.setValue(synth.getPadParameter(selectedPad, "level"), juce::dontSendNotification);
    panKnob.setValue(synth.getPadParameter(selectedPad, "pan"), juce::dontSendNotification);
}

void DrumSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &kitSelector)
    {
        int index = box->getSelectedId() - 1;
        auto kits = synth.getAvailableKits();
        if (index >= 0 && index < kits.size())
        {
            synth.loadKit(kits[index]);
            updatePadControls();
        }
    }
}

void DrumSynthEditor::buttonClicked(juce::Button* button)
{
    // Check if it's a pad button
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (button == &padButtons[i])
        {
            selectPad(i);

            // Also trigger the sound for preview
            synth.allNotesOff();
            // Get MIDI note for this pad (would need to expose this)
            int midiNote = 36 + i; // Simple mapping
            synth.noteOn(midiNote, 0.8f);
            return;
        }
    }
}

void DrumSynthEditor::refreshFromSynth()
{
    // Master volume
    if (auto* param = synth.getParameterInfo("volume"))
        masterVolume.setValue(param->value, juce::dontSendNotification);

    // Kit selector
    populateKits();

    // Pad controls
    updatePadControls();
}

void DrumSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int labelHeight = 16;
    const int knobSpacing = 8;
    const int innerMargin = 6;

    // Add Kit selector to header (between preset and master volume)
    auto headerArea = getLocalBounds().removeFromTop(HEADER_HEIGHT);
    headerArea.removeFromLeft(SECTION_PADDING);

    // Position kit selector next to preset selector (which is already laid out by base)
    auto kitArea = headerArea.removeFromLeft(160).reduced(0, 8);
    kitLabel.setBounds(kitArea.removeFromTop(labelHeight));
    kitSelector.setBounds(kitArea.withHeight(24));

    // Split content into two sections: Pad Grid (55%) | Pad Controls (45%)
    const int padGridWidth = static_cast<int>(area.getWidth() * 0.55f);
    auto padGridArea = area.removeFromLeft(padGridWidth).reduced(SECTION_PADDING);
    auto controlsArea = area.reduced(SECTION_PADDING);

    // Store divider position
    padGridDividerX = padGridArea.getRight() + SECTION_PADDING;

    //==========================================================================
    // PAD GRID SECTION (4x4 grid of buttons)
    {
        const int padW = (padGridArea.getWidth() - 3 * innerMargin) / 4;
        const int padH = (padGridArea.getHeight() - 3 * innerMargin) / 4;

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                int index = row * 4 + col;
                int x = padGridArea.getX() + col * (padW + innerMargin);
                int y = padGridArea.getY() + row * (padH + innerMargin);
                padButtons[index].setBounds(x, y, padW, padH);
            }
        }
    }

    //==========================================================================
    // PAD CONTROLS SECTION
    {
        padControlsLabel.setBounds(controlsArea.removeFromTop(labelHeight));
        controlsArea.removeFromTop(knobSpacing);

        // Arrange knobs in two rows
        auto row1 = controlsArea.removeFromTop(KNOB_SIZE + 20);
        pitchKnob.setBounds(row1.removeFromLeft(KNOB_SIZE).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE));
        row1.removeFromLeft(knobSpacing);
        decayKnob.setBounds(row1.removeFromLeft(KNOB_SIZE).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE));
        row1.removeFromLeft(knobSpacing);
        toneKnob.setBounds(row1.removeFromLeft(KNOB_SIZE).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE));

        controlsArea.removeFromTop(knobSpacing);
        auto row2 = controlsArea.removeFromTop(KNOB_SIZE + 20);
        levelKnob.setBounds(row2.removeFromLeft(KNOB_SIZE).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE));
        row2.removeFromLeft(knobSpacing);
        panKnob.setBounds(row2.removeFromLeft(KNOB_SIZE).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE));
    }
}

void DrumSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw vertical divider between pad grid and pad controls
    if (padGridDividerX > 0)
    {
        drawVerticalDivider(g, padGridDividerX, area.getY(), area.getBottom());
    }
}
