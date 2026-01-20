#include "FMSynthEditor.h"

FMSynthEditor::FMSynthEditor(FMSynth& s)
    : SynthEditorBase(), synth(s), rowDividerY(0)
{
    // Setup preset selector and master volume (from base class)
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "volume", "Volume");

    //==========================================================================
    // ALGORITHM & OPERATORS SECTION
    //==========================================================================

    // Algorithm
    createSectionLabel(algorithmLabel, "ALGORITHM");
    addAndMakeVisible(algorithmLabel);
    setupComboBox(algorithmSelector, "algorithm");

    // Carrier
    createSectionLabel(carrierLabel, "CARRIER");
    addAndMakeVisible(carrierLabel);
    setupKnob(carrierRatio, "carrier_ratio", "Ratio");

    // Modulator 1
    createSectionLabel(mod1Label, "MODULATOR 1");
    addAndMakeVisible(mod1Label);
    setupKnob(mod1Ratio, "mod1_ratio", "Ratio");
    setupKnob(mod1Index, "mod1_index", "Index");

    // Modulator 2
    createSectionLabel(mod2Label, "MODULATOR 2");
    addAndMakeVisible(mod2Label);
    setupKnob(mod2Ratio, "mod2_ratio", "Ratio");
    setupKnob(mod2Index, "mod2_index", "Index");

    //==========================================================================
    // FEEDBACK & ENVELOPES SECTION
    //==========================================================================

    // Feedback
    createSectionLabel(feedbackLabel, "FEEDBACK");
    addAndMakeVisible(feedbackLabel);
    setupKnob(feedbackKnob, "feedback", "Amount");

    // Amp Envelope
    createSectionLabel(ampEnvLabel, "AMP ENV");
    addAndMakeVisible(ampEnvLabel);
    setupKnob(ampAttack, "amp_attack", "A", " s");
    setupKnob(ampDecay, "amp_decay", "D", " s");
    setupKnob(ampSustain, "amp_sustain", "S");
    setupKnob(ampRelease, "amp_release", "R", " s");

    // Mod 1 Envelope
    createSectionLabel(mod1EnvLabel, "MOD 1 ENV");
    addAndMakeVisible(mod1EnvLabel);
    setupKnob(mod1Attack, "mod1_attack", "A", " s");
    setupKnob(mod1Decay, "mod1_decay", "D", " s");
    setupKnob(mod1Sustain, "mod1_sustain", "S");
    setupKnob(mod1Release, "mod1_release", "R", " s");

    // Mod 2 Envelope
    createSectionLabel(mod2EnvLabel, "MOD 2 ENV");
    addAndMakeVisible(mod2EnvLabel);
    setupKnob(mod2Attack, "mod2_attack", "A", " s");
    setupKnob(mod2Decay, "mod2_decay", "D", " s");
    setupKnob(mod2Sustain, "mod2_sustain", "S");
    setupKnob(mod2Release, "mod2_release", "R", " s");

    // Initial refresh
    refreshFromSynth();
}

FMSynthEditor::~FMSynthEditor()
{
    presetSelector.removeListener(this);
    algorithmSelector.removeListener(this);
}

void FMSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void FMSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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

void FMSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &algorithmSelector)
    {
        synth.setParameterEnum("algorithm", index);
    }
}

void FMSynthEditor::populatePresets()
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

void FMSynthEditor::refreshFromSynth()
{
    // Helper lambda to refresh a knob
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    // Helper lambda to refresh a combo box
    auto refreshCombo = [this](juce::ComboBox& box, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    };

    // Master
    refreshKnob(masterVolume, "volume");

    // Algorithm
    refreshCombo(algorithmSelector, "algorithm");

    // Carrier
    refreshKnob(carrierRatio, "carrier_ratio");

    // Modulator 1
    refreshKnob(mod1Ratio, "mod1_ratio");
    refreshKnob(mod1Index, "mod1_index");

    // Modulator 2
    refreshKnob(mod2Ratio, "mod2_ratio");
    refreshKnob(mod2Index, "mod2_index");

    // Feedback
    refreshKnob(feedbackKnob, "feedback");

    // Amp Envelope
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // Mod 1 Envelope
    refreshKnob(mod1Attack, "mod1_attack");
    refreshKnob(mod1Decay, "mod1_decay");
    refreshKnob(mod1Sustain, "mod1_sustain");
    refreshKnob(mod1Release, "mod1_release");

    // Mod 2 Envelope
    refreshKnob(mod2Attack, "mod2_attack");
    refreshKnob(mod2Decay, "mod2_decay");
    refreshKnob(mod2Sustain, "mod2_sustain");
    refreshKnob(mod2Release, "mod2_release");

    // Update preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void FMSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    // Clear previous dividers
    sectionDividers.clear();

    const int labelHeight = 16;
    const int comboHeight = 24;
    const int labelGap = 6;
    const int knobRowGap = 8;

    // Two rows with horizontal divider
    // Row 1: 50% - ALGORITHM (20%) | CARRIER (20%) | MOD 1 (20%) | MOD 2 (20%) | FEEDBACK (20%)
    // Row 2: 50% - AMP ENV (33%) | MOD 1 ENV (33%) | MOD 2 ENV (34%)

    int row1Height = area.getHeight() / 2;
    int row2Height = area.getHeight() - row1Height;

    auto row1Area = area.removeFromTop(row1Height);
    rowDividerY = row1Area.getBottom();
    auto row2Area = area;

    //==========================================================================
    // ROW 1: Algorithm, Carrier, Modulators, Feedback
    //==========================================================================
    {
        const int totalWidth = row1Area.getWidth();
        const int algorithmWidth = totalWidth * 20 / 100;
        const int carrierWidth = totalWidth * 20 / 100;
        const int mod1Width = totalWidth * 20 / 100;
        const int mod2Width = totalWidth * 20 / 100;
        const int feedbackWidth = totalWidth - algorithmWidth - carrierWidth - mod1Width - mod2Width;

        auto layoutRow = row1Area;

        // Algorithm
        {
            auto section = layoutRow.removeFromLeft(algorithmWidth).reduced(SECTION_PADDING, 0);
            algorithmLabel.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap);
            algorithmSelector.setBounds(section.removeFromTop(comboHeight));
        }
        sectionDividers.push_back(layoutRow.getX());

        // Carrier
        {
            auto section = layoutRow.removeFromLeft(carrierWidth).reduced(SECTION_PADDING, 0);
            carrierLabel.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap + comboHeight + knobRowGap); // Align with algorithm
            carrierRatio.setBounds(section.removeFromTop(KNOB_SIZE + 20).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
        sectionDividers.push_back(layoutRow.getX());

        // Modulator 1
        {
            auto section = layoutRow.removeFromLeft(mod1Width).reduced(SECTION_PADDING, 0);
            mod1Label.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap + comboHeight + knobRowGap); // Align with algorithm

            auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
            int knobSpacing = knobRow.getWidth() / 2;
            mod1Ratio.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            mod1Index.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
        sectionDividers.push_back(layoutRow.getX());

        // Modulator 2
        {
            auto section = layoutRow.removeFromLeft(mod2Width).reduced(SECTION_PADDING, 0);
            mod2Label.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap + comboHeight + knobRowGap); // Align with algorithm

            auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
            int knobSpacing = knobRow.getWidth() / 2;
            mod2Ratio.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            mod2Index.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
        sectionDividers.push_back(layoutRow.getX());

        // Feedback
        {
            auto section = layoutRow.removeFromLeft(feedbackWidth).reduced(SECTION_PADDING, 0);
            feedbackLabel.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap + comboHeight + knobRowGap); // Align with algorithm
            feedbackKnob.setBounds(section.removeFromTop(KNOB_SIZE + 20).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
    }

    //==========================================================================
    // ROW 2: Envelopes
    //==========================================================================
    {
        const int totalWidth = row2Area.getWidth();
        const int ampEnvWidth = totalWidth * 33 / 100;
        const int mod1EnvWidth = totalWidth * 33 / 100;
        const int mod2EnvWidth = totalWidth - ampEnvWidth - mod1EnvWidth;

        auto layoutRow = row2Area;

        // Helper for envelope sections (4 knobs each)
        auto layoutEnvelope = [&](juce::Label& label, RotaryKnob& a, RotaryKnob& d,
                                   RotaryKnob& s, RotaryKnob& r, int width)
        {
            auto section = layoutRow.removeFromLeft(width).reduced(SECTION_PADDING, 0);
            label.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap);

            auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
            int knobSpacing = knobRow.getWidth() / 4;
            a.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            d.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            s.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            r.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));

            sectionDividers.push_back(layoutRow.getX());
        };

        layoutEnvelope(ampEnvLabel, ampAttack, ampDecay, ampSustain, ampRelease, ampEnvWidth);
        layoutEnvelope(mod1EnvLabel, mod1Attack, mod1Decay, mod1Sustain, mod1Release, mod1EnvWidth);
        layoutEnvelope(mod2EnvLabel, mod2Attack, mod2Decay, mod2Sustain, mod2Release, mod2EnvWidth);
    }
}

void FMSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw horizontal divider between rows
    drawHorizontalDivider(g, area.getX(), area.getRight(), rowDividerY);

    // Draw vertical dividers between sections
    // Skip last 3 dividers as they're from row 2 and shouldn't extend to full height
    int row1Dividers = 4; // ALGORITHM | CARRIER | MOD1 | MOD2 | FEEDBACK
    for (int i = 0; i < row1Dividers && i < static_cast<int>(sectionDividers.size()); i++)
    {
        drawVerticalDivider(g, sectionDividers[i], area.getY(), rowDividerY);
    }

    // Draw row 2 dividers (only below horizontal divider)
    for (size_t i = row1Dividers; i < sectionDividers.size(); i++)
    {
        drawVerticalDivider(g, sectionDividers[i], rowDividerY, area.getBottom());
    }
}
