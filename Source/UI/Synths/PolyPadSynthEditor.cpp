#include "PolyPadSynthEditor.h"

PolyPadSynthEditor::PolyPadSynthEditor(PolyPadSynth& s)
    : SynthEditorBase(), synth(s), rowDividerY(0)
{
    // Setup master volume
    setupKnob(masterVolume, "volume", "Volume");

    // Populate and setup preset selector
    populatePresets();
    presetSelector.addListener(this);

    //==========================================================================
    // OSCILLATOR SECTION
    //==========================================================================
    createSectionLabel(oscLabel, "OSCILLATORS");
    addAndMakeVisible(oscLabel);
    setupComboBox(osc1Wave, "osc1_wave");
    setupComboBox(osc2Wave, "osc2_wave");
    setupKnob(osc2Detune, "osc2_detune", "Detune", " ct");
    setupKnob(oscMix, "osc_mix", "Mix");

    //==========================================================================
    // FILTER SECTION
    //==========================================================================
    createSectionLabel(filterLabel, "FILTER");
    addAndMakeVisible(filterLabel);
    setupComboBox(filterType, "filter_type");
    setupKnob(filterCutoff, "filter_cutoff", "Cutoff", " Hz");
    setupKnob(filterResonance, "filter_resonance", "Reso");
    setupKnob(filterEnvAmount, "filter_env_amount", "Env Amt", " Hz");

    //==========================================================================
    // CHORUS SECTION
    //==========================================================================
    createSectionLabel(chorusLabel, "CHORUS");
    addAndMakeVisible(chorusLabel);
    setupKnob(chorusRate, "chorus_rate", "Rate", " Hz");
    setupKnob(chorusDepth, "chorus_depth", "Depth");
    setupKnob(chorusMix, "chorus_wet", "Mix");

    //==========================================================================
    // AMP ENVELOPE SECTION
    //==========================================================================
    createSectionLabel(ampEnvLabel, "AMP ENVELOPE");
    addAndMakeVisible(ampEnvLabel);
    setupKnob(ampAttack, "amp_attack", "A", " s");
    setupKnob(ampDecay, "amp_decay", "D", " s");
    setupKnob(ampSustain, "amp_sustain", "S");
    setupKnob(ampRelease, "amp_release", "R", " s");

    //==========================================================================
    // FILTER ENVELOPE SECTION
    //==========================================================================
    createSectionLabel(filterEnvLabel, "FILTER ENVELOPE");
    addAndMakeVisible(filterEnvLabel);
    setupKnob(filterAttack, "filter_attack", "A", " s");
    setupKnob(filterDecay, "filter_decay", "D", " s");
    setupKnob(filterSustain, "filter_sustain", "S");
    setupKnob(filterRelease, "filter_release", "R", " s");

    // Initial refresh
    refreshFromSynth();
}

PolyPadSynthEditor::~PolyPadSynthEditor()
{
    presetSelector.removeListener(this);
    osc1Wave.removeListener(this);
    osc2Wave.removeListener(this);
    filterType.removeListener(this);
}

void PolyPadSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void PolyPadSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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


void PolyPadSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &osc1Wave)
        synth.setParameterEnum("osc1_wave", index);
    else if (box == &osc2Wave)
        synth.setParameterEnum("osc2_wave", index);
    else if (box == &filterType)
        synth.setParameterEnum("filter_type", index);
}

void PolyPadSynthEditor::populatePresets()
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

void PolyPadSynthEditor::refreshFromSynth()
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

    // Oscillators
    refreshCombo(osc1Wave, "osc1_wave");
    refreshCombo(osc2Wave, "osc2_wave");
    refreshKnob(osc2Detune, "osc2_detune");
    refreshKnob(oscMix, "osc_mix");

    // Filter
    refreshCombo(filterType, "filter_type");
    refreshKnob(filterCutoff, "filter_cutoff");
    refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterEnvAmount, "filter_env_amount");

    // Chorus
    refreshKnob(chorusRate, "chorus_rate");
    refreshKnob(chorusDepth, "chorus_depth");
    refreshKnob(chorusMix, "chorus_wet");

    // Amp Envelope
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // Filter Envelope
    refreshKnob(filterAttack, "filter_attack");
    refreshKnob(filterDecay, "filter_decay");
    refreshKnob(filterSustain, "filter_sustain");
    refreshKnob(filterRelease, "filter_release");

    // Update preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void PolyPadSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    sectionDividers.clear();

    const int labelHeight = 14;
    const int labelGap = 4;
    const int comboHeight = 24;
    const int knobRowGap = 8;

    //==========================================================================
    // ROW 1: OSCILLATORS (30%) | FILTER (35%) | CHORUS (35%)
    //==========================================================================
    const int totalWidth = area.getWidth();
    const int row1Height = area.getHeight() * 30 / 100;
    auto row1Area = area.removeFromTop(row1Height);
    rowDividerY = area.getY();

    {
        const int oscWidth = totalWidth * 30 / 100;
        const int filterWidth = totalWidth * 35 / 100;
        const int chorusWidth = totalWidth - oscWidth - filterWidth;

        auto layoutRow = row1Area;

        // Oscillators
        {
            auto section = layoutRow.removeFromLeft(oscWidth).reduced(SECTION_PADDING, 0);
            oscLabel.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap);

            auto comboRow = section.removeFromTop(comboHeight);
            int comboW = (comboRow.getWidth() - 8) / 2;
            osc1Wave.setBounds(comboRow.removeFromLeft(comboW));
            comboRow.removeFromLeft(8);
            osc2Wave.setBounds(comboRow);

            section.removeFromTop(knobRowGap);
            auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
            int knobSpacing = knobRow.getWidth() / 2;
            osc2Detune.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            oscMix.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
        sectionDividers.push_back(layoutRow.getX());

        // Filter
        {
            auto section = layoutRow.removeFromLeft(filterWidth).reduced(SECTION_PADDING, 0);
            filterLabel.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap);
            filterType.setBounds(section.removeFromTop(comboHeight));

            section.removeFromTop(knobRowGap);
            auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
            int knobSpacing = knobRow.getWidth() / 3;
            filterCutoff.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            filterResonance.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            filterEnvAmount.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
        sectionDividers.push_back(layoutRow.getX());

        // Chorus
        {
            auto section = layoutRow.removeFromLeft(chorusWidth).reduced(SECTION_PADDING, 0);
            chorusLabel.setBounds(section.removeFromTop(labelHeight));
            section.removeFromTop(labelGap + comboHeight + knobRowGap); // Align with filter

            auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
            int knobSpacing = knobRow.getWidth() / 3;
            chorusRate.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            chorusDepth.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
            chorusMix.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        }
    }

    //==========================================================================
    // ROW 2: AMP ENVELOPE (50%) | FILTER ENVELOPE (50%)
    //==========================================================================
    auto row2Area = area;

    {
        const int ampEnvWidth = totalWidth * 50 / 100;
        const int filterEnvWidth = totalWidth - ampEnvWidth;

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
        layoutEnvelope(filterEnvLabel, filterAttack, filterDecay, filterSustain, filterRelease, filterEnvWidth);
    }
}

void PolyPadSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw horizontal divider between rows
    drawHorizontalDivider(g, area.getX(), area.getRight(), rowDividerY);

    // Draw vertical dividers between sections
    // Row 1 has 2 dividers (3 sections), row 2 has 1 divider (2 sections)
    int row1Dividers = 2; // OSCILLATORS | FILTER | CHORUS
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
