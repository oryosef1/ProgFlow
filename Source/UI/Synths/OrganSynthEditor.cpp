#include "OrganSynthEditor.h"

OrganSynthEditor::OrganSynthEditor(OrganSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Setup preset selector and master volume (from base class)
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "volume", "Volume");

    // Drawbars Section
    createSectionLabel(drawbarsLabel, "DRAWBARS");
    addAndMakeVisible(drawbarsLabel);

    const char* drawbarNames[9] = {"16'", "5⅓'", "8'", "4'", "2⅔'", "2'", "1⅗'", "1⅓'", "1'"};
    for (int i = 0; i < 9; ++i)
    {
        juce::String paramId = "drawbar_" + juce::String(i);
        setupKnob(drawbars[i], paramId, drawbarNames[i]);

        drawbarLabels[i].setText(drawbarNames[i], juce::dontSendNotification);
        drawbarLabels[i].setFont(juce::Font(10.0f));
        drawbarLabels[i].setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
        drawbarLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(drawbarLabels[i]);
    }

    // Percussion Section
    createSectionLabel(percussionLabel, "PERCUSSION");
    addAndMakeVisible(percussionLabel);

    setupComboBox(percussionType, "percussion");
    setupComboBox(percussionDecay, "percussion_decay");
    setupComboBox(percussionHarmonic, "percussion_harmonic");

    // Rotary Section
    createSectionLabel(rotaryLabel, "ROTARY");
    addAndMakeVisible(rotaryLabel);

    setupComboBox(rotarySpeed, "rotary_speed");
    setupKnob(rotaryDepth, "rotary_depth", "Depth");

    // Drive Section
    createSectionLabel(driveLabel, "DRIVE");
    addAndMakeVisible(driveLabel);
    setupKnob(driveKnob, "drive", "Drive");

    // Key Click Section
    createSectionLabel(keyClickLabel, "KEY CLICK");
    addAndMakeVisible(keyClickLabel);
    setupKnob(keyClickKnob, "key_click", "Click");

    refreshFromSynth();
}

OrganSynthEditor::~OrganSynthEditor()
{
    percussionType.removeListener(this);
    percussionDecay.removeListener(this);
    percussionHarmonic.removeListener(this);
    rotarySpeed.removeListener(this);
}

void OrganSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
                                  const juce::String& label, const juce::String& suffix)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);

    if (auto* param = synth.getParameterInfo(paramId))
    {
        knob.setRange(param->minValue, param->maxValue, param->step);
        knob.setDefaultValue(param->defaultValue);
        knob.setValue(param->value, juce::dontSendNotification);
    }

    knob.onValueChange = [this, paramId](float value)
    {
        synth.setParameter(paramId, value);
    };

    addAndMakeVisible(knob);
}

void OrganSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
{
    if (auto* param = synth.getParameterInfo(paramId))
    {
        int id = 1;
        for (const auto& option : param->options)
        {
            box.addItem(option, id++);
        }
        box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    }

    box.addListener(this);
    addAndMakeVisible(box);
}

void OrganSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &percussionType)
    {
        synth.setParameterEnum("percussion", index);
    }
    else if (box == &percussionDecay)
    {
        synth.setParameterEnum("percussion_decay", index);
    }
    else if (box == &percussionHarmonic)
    {
        synth.setParameterEnum("percussion_harmonic", index);
    }
    else if (box == &rotarySpeed)
    {
        synth.setParameterEnum("rotary_speed", index);
    }
}

void OrganSynthEditor::populatePresets()
{
    presetSelector.clear();

    auto presets = synth.getPresets();
    int id = 1;
    for (const auto& preset : presets)
    {
        presetSelector.addItem(preset.name, id++);
    }

    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
    {
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
    }
    else if (!presets.empty())
    {
        synth.loadPreset(0);
        presetSelector.setSelectedId(1, juce::dontSendNotification);
    }
}

void OrganSynthEditor::refreshFromSynth()
{
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    auto refreshCombo = [this](juce::ComboBox& box, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    };

    // Master
    refreshKnob(masterVolume, "volume");

    // Drawbars
    for (int i = 0; i < 9; ++i)
    {
        refreshKnob(drawbars[i], "drawbar_" + juce::String(i));
    }

    // Percussion
    refreshCombo(percussionType, "percussion");
    refreshCombo(percussionDecay, "percussion_decay");
    refreshCombo(percussionHarmonic, "percussion_harmonic");

    // Rotary
    refreshCombo(rotarySpeed, "rotary_speed");
    refreshKnob(rotaryDepth, "rotary_depth");

    // Drive & Key Click
    refreshKnob(driveKnob, "drive");
    refreshKnob(keyClickKnob, "key_click");

    // Preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void OrganSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int gap = ProgFlowSpacing::SM;
    const int sectionPad = SECTION_PADDING;
    const int knobSize = 70;
    const int labelHeight = 18;
    const int comboHeight = 24;

    // Calculate row heights proportionally (55% for drawbars, 45% for controls)
    const int row1Height = area.getHeight() * 55 / 100;
    const int row2Height = area.getHeight() - row1Height - gap;

    //==========================================================================
    // ROW 1: Drawbars (full width)
    //==========================================================================
    auto row1 = area.removeFromTop(row1Height).reduced(sectionPad);
    row1Bottom = row1.getBottom() + gap / 2;  // Store for divider drawing

    drawbarsLabel.setBounds(row1.removeFromTop(labelHeight));
    row1.removeFromTop(4);

    // Hide duplicate labels - knobs already show their labels
    for (int i = 0; i < 9; ++i)
        drawbarLabels[i].setVisible(false);

    // 9 drawbar knobs spread across the width
    int drawbarWidth = row1.getWidth() / 9;
    for (int i = 0; i < 9; ++i)
    {
        auto col = row1.removeFromLeft(drawbarWidth);
        drawbars[i].setBounds(col.withSizeKeepingCentre(knobSize - 10, knobSize));
    }

    area.removeFromTop(gap);  // Gap between rows

    //==========================================================================
    // ROW 2: Percussion | Rotary | Drive | Key Click (4 columns at 25% each)
    //==========================================================================
    auto row2 = area.removeFromTop(row2Height);
    int colWidth = row2.getWidth() / 4;

    row2Dividers.clear();

    // Percussion (3 combo boxes)
    {
        auto col = row2.removeFromLeft(colWidth).reduced(sectionPad);
        percussionLabel.setBounds(col.removeFromTop(labelHeight));
        col.removeFromTop(4);

        percussionType.setBounds(col.removeFromTop(comboHeight));
        col.removeFromTop(4);
        percussionHarmonic.setBounds(col.removeFromTop(comboHeight));
        col.removeFromTop(4);
        percussionDecay.setBounds(col.removeFromTop(comboHeight));
    }
    row2Dividers.push_back(row2.getX());

    // Rotary (1 combo + 1 knob)
    {
        auto col = row2.removeFromLeft(colWidth).reduced(sectionPad);
        rotaryLabel.setBounds(col.removeFromTop(labelHeight));
        col.removeFromTop(4);

        rotarySpeed.setBounds(col.removeFromTop(comboHeight));
        col.removeFromTop(8);
        rotaryDepth.setBounds(col.withSizeKeepingCentre(knobSize, knobSize));
    }
    row2Dividers.push_back(row2.getX());

    // Drive (1 knob)
    {
        auto col = row2.removeFromLeft(colWidth).reduced(sectionPad);
        driveLabel.setBounds(col.removeFromTop(labelHeight));
        col.removeFromTop(4);
        driveKnob.setBounds(col.withSizeKeepingCentre(knobSize, knobSize));
    }
    row2Dividers.push_back(row2.getX());

    // Key Click (1 knob)
    {
        auto col = row2.reduced(sectionPad);
        keyClickLabel.setBounds(col.removeFromTop(labelHeight));
        col.removeFromTop(4);
        keyClickKnob.setBounds(col.withSizeKeepingCentre(knobSize, knobSize));
    }
}

void OrganSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Horizontal divider after drawbars row
    if (row1Bottom > 0)
    {
        drawHorizontalDivider(g, area.getX(), area.getRight(), row1Bottom);
    }

    // Vertical dividers in row 2 (between the 4 sections)
    for (int x : row2Dividers)
    {
        drawVerticalDivider(g, x, row1Bottom, area.getBottom());
    }
}
