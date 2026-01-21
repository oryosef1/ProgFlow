#include "DrumSynthEditor.h"

DrumSynthEditor::DrumSynthEditor(DrumSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Hide the base class preset selector - Drums uses kit selector instead
    presetSelector.setVisible(false);
    presetLabel.setVisible(false);

    // Kit selector in header
    kitSelector.addListener(this);
    addAndMakeVisible(kitSelector);
    populateKits();

    // Master Volume
    masterVolume.setLabel("Volume");
    masterVolume.setTooltipText("Master drum kit volume");
    if (auto* param = synth.getParameterInfo("volume"))
    {
        masterVolume.setRange(param->minValue, param->maxValue, param->step);
        masterVolume.setDefaultValue(param->defaultValue);
        masterVolume.setValue(param->value, juce::dontSendNotification);
    }
    masterVolume.onValueChange = [this](float value) { synth.setParameter("volume", value); };

    //==========================================================================
    // CARD PANELS (Saturn design - no headers for compact layout)
    //==========================================================================
    padGridCard.setShowHeader(false);
    padGridCard.setPadding(6);
    addAndMakeVisible(padGridCard);

    padControlsCard.setShowHeader(false);
    padControlsCard.setPadding(6);
    addAndMakeVisible(padControlsCard);

    // Pad Buttons (4x4 grid)
    for (int i = 0; i < NUM_PADS; ++i)
    {
        padButtons[i].setButtonText(synth.getPadName(i));
        padButtons[i].addListener(this);
        padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        padGridCard.addAndMakeVisible(padButtons[i]);
    }

    //==========================================================================
    // PAD CONTROLS
    //==========================================================================
    setupKnob(pitchKnob, "Pitch", "", "Pitch shift - change drum tone");
    padControlsCard.addAndMakeVisible(pitchKnob);
    setupKnob(decayKnob, "Decay", "", "Decay time - how long sound rings out");
    padControlsCard.addAndMakeVisible(decayKnob);
    setupKnob(toneKnob, "Tone", "", "Tone control - brightness of the drum");
    padControlsCard.addAndMakeVisible(toneKnob);
    setupKnob(levelKnob, "Level", "", "Individual pad volume level");
    padControlsCard.addAndMakeVisible(levelKnob);
    setupKnob(panKnob, "Pan", "", "Stereo position (-1 = left, +1 = right)");
    padControlsCard.addAndMakeVisible(panKnob);

    // Set up knob callbacks
    pitchKnob.onValueChange = [this](float value) { synth.setPadParameter(selectedPad, "pitch", value); };
    decayKnob.onValueChange = [this](float value) { synth.setPadParameter(selectedPad, "decay", value); };
    toneKnob.onValueChange = [this](float value) { synth.setPadParameter(selectedPad, "tone", value); };
    levelKnob.onValueChange = [this](float value) { synth.setPadParameter(selectedPad, "level", value); };
    panKnob.onValueChange = [this](float value) { synth.setPadParameter(selectedPad, "pan", value); };

    selectPad(0);
    refreshFromSynth();
}

DrumSynthEditor::~DrumSynthEditor()
{
    stopTimer();
    kitSelector.removeListener(this);
    for (auto& btn : padButtons)
        btn.removeListener(this);
}

void DrumSynthEditor::timerCallback()
{
    stopTimer();
    flashingPad = -1;
    updatePadAppearance();
}

void DrumSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& label,
                                 const juce::String& suffix, const juce::String& description)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);
    knob.setRange(0.0f, 1.0f, 0.01f);
    knob.setDefaultValue(0.5f);

    if (description.isNotEmpty())
        knob.setTooltipText(description);

    addAndMakeVisible(knob);
}

void DrumSynthEditor::populateKits()
{
    kitSelector.clear();

    auto kits = synth.getAvailableKits();
    int id = 1;
    for (const auto& kit : kits)
        kitSelector.addItem(kit, id++);

    juce::String currentKit = synth.getCurrentKit();
    int index = kits.indexOf(currentKit);
    kitSelector.setSelectedId(index >= 0 ? index + 1 : 1, juce::dontSendNotification);
}

void DrumSynthEditor::selectPad(int padIndex)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    selectedPad = padIndex;
    updatePadAppearance();
    updatePadControls();

    // Update card title to show selected pad
    padControlsCard.setTitle("PAD: " + synth.getPadName(padIndex));
}

void DrumSynthEditor::flashPad(int padIndex)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    flashingPad = padIndex;
    updatePadAppearance();

    // Start timer to reset flash after 150ms
    startTimer(150);
}

void DrumSynthEditor::updatePadAppearance()
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (i == flashingPad)
        {
            // Currently playing - bright highlight
            padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
        }
        else if (i == selectedPad)
        {
            // Selected for editing - subtle highlight (darker shade)
            padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgHover());
        }
        else
        {
            // Normal state
            padButtons[i].setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        }
    }
}

void DrumSynthEditor::updatePadControls()
{
    pitchKnob.setRange(0.5f, 2.0f, 0.01f);
    pitchKnob.setDefaultValue(1.0f);
    panKnob.setRange(-1.0f, 1.0f, 0.01f);
    panKnob.setDefaultValue(0.0f);

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
            // Update pad names
            for (int i = 0; i < NUM_PADS; ++i)
                padButtons[i].setButtonText(synth.getPadName(i));
            updatePadControls();
        }
    }
}

void DrumSynthEditor::buttonClicked(juce::Button* button)
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (button == &padButtons[i])
        {
            selectPad(i);      // Select for editing
            flashPad(i);       // Flash briefly when played
            synth.allNotesOff();
            // Use the actual MIDI note assigned to this pad
            int midiNote = synth.getPadMidiNote(i);
            if (midiNote >= 0)
                synth.noteOn(midiNote, 0.8f);
            return;
        }
    }
}

void DrumSynthEditor::refreshFromSynth()
{
    if (auto* param = synth.getParameterInfo("volume"))
        masterVolume.setValue(param->value, juce::dontSendNotification);

    populateKits();
    updatePadControls();
}

void DrumSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int cardGap = 6;
    const int knobHeight = RotaryKnob::TOTAL_HEIGHT;

    // Add Kit selector to header area (using space where preset selector was)
    auto headerBounds = getLocalBounds().removeFromTop(HEADER_HEIGHT).reduced(SECTION_PADDING, 16);
    kitSelector.setBounds(headerBounds.removeFromLeft(150).withHeight(28));

    // Two cards: Pad Grid (55%) | Pad Controls (45%)
    int padGridWidth = area.getWidth() * 55 / 100;

    auto padGridBounds = area.removeFromLeft(padGridWidth);
    padGridCard.setBounds(padGridBounds);
    auto gridContent = padGridCard.getContentArea();

    area.removeFromLeft(cardGap);
    padControlsCard.setBounds(area);
    auto controlsContent = padControlsCard.getContentArea();

    //==========================================================================
    // PAD GRID (4x4)
    //==========================================================================
    {
        const int innerMargin = 4;
        const int padW = (gridContent.getWidth() - 3 * innerMargin) / 4;
        const int padH = (gridContent.getHeight() - 3 * innerMargin) / 4;

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                int index = row * 4 + col;
                int x = gridContent.getX() + col * (padW + innerMargin);
                int y = gridContent.getY() + row * (padH + innerMargin);
                padButtons[index].setBounds(x, y, padW, padH);
            }
        }
    }

    //==========================================================================
    // PAD CONTROLS (5 knobs in a row)
    //==========================================================================
    {
        int knobSpacing = controlsContent.getWidth() / 5;
        int centreY = controlsContent.getCentreY();

        pitchKnob.setBounds(controlsContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        decayKnob.setBounds(controlsContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        toneKnob.setBounds(controlsContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        levelKnob.setBounds(controlsContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        panKnob.setBounds(controlsContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    }
}

void DrumSynthEditor::drawDividers(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // No dividers needed - CardPanels handle their own styling
}
