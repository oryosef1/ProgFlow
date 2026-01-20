#include "AnalogSynthEditor.h"

AnalogSynthEditor::AnalogSynthEditor(AnalogSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Setup preset selector and master volume (from base class)
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "master_volume", "Volume");

    //==========================================================================
    // OSCILLATOR 1
    //==========================================================================
    createSectionLabel(osc1Label, "OSC 1");
    addAndMakeVisible(osc1Label);
    setupWaveSelector(osc1Wave, "osc1_wave");
    setupKnob(osc1Octave, "osc1_octave", "Semi");
    setupKnob(osc1Detune, "osc1_detune", "Fine", " ct");

    //==========================================================================
    // OSCILLATOR 2
    //==========================================================================
    createSectionLabel(osc2Label, "OSC 2");
    addAndMakeVisible(osc2Label);
    setupWaveSelector(osc2Wave, "osc2_wave");
    setupKnob(osc2Octave, "osc2_octave", "Semi");
    setupKnob(osc2Detune, "osc2_detune", "Detune", " ct");

    //==========================================================================
    // FILTER
    //==========================================================================
    createSectionLabel(filterLabel, "FILTER");
    addAndMakeVisible(filterLabel);
    setupComboBox(filterType, "filter_type");
    setupKnob(filterCutoff, "filter_cutoff", "Cut", " Hz");
    setupKnob(filterResonance, "filter_resonance", "Res");
    setupKnob(filterEnvAmount, "filter_env_amount", "Env", " Hz");

    //==========================================================================
    // AMP ENVELOPE
    //==========================================================================
    createSectionLabel(ampEnvLabel, "AMP ENV");
    addAndMakeVisible(ampEnvLabel);
    setupKnob(ampAttack, "amp_attack", "A", " s");
    setupKnob(ampDecay, "amp_decay", "D", " s");
    setupKnob(ampSustain, "amp_sustain", "S");
    setupKnob(ampRelease, "amp_release", "R", " s");

    //==========================================================================
    // FILTER ENVELOPE
    //==========================================================================
    createSectionLabel(filterEnvLabel, "FLT ENV");
    addAndMakeVisible(filterEnvLabel);
    setupKnob(filterAttack, "filter_attack", "A", " s");
    setupKnob(filterDecay, "filter_decay", "D", " s");
    setupKnob(filterSustain, "filter_sustain", "S");
    setupKnob(filterRelease, "filter_release", "R", " s");

    // Initial refresh
    refreshFromSynth();
}

AnalogSynthEditor::~AnalogSynthEditor()
{
    filterType.removeListener(this);
}

void AnalogSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void AnalogSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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

void AnalogSynthEditor::setupWaveSelector(WaveSelector& selector, const juce::String& paramId)
{
    if (auto* param = synth.getParameterInfo(paramId))
    {
        selector.setSelectedIndex(param->enumIndex, juce::dontSendNotification);
    }

    selector.onSelectionChanged = [this, paramId](int index)
    {
        synth.setParameterEnum(paramId, index);
    };

    addAndMakeVisible(selector);
}


void AnalogSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &filterType)
    {
        synth.setParameterEnum("filter_type", index);
    }
}

void AnalogSynthEditor::populatePresets()
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
        // Auto-select the first preset if none selected
        synth.loadPreset(0);
        presetSelector.setSelectedId(1, juce::dontSendNotification);
    }
}

void AnalogSynthEditor::refreshFromSynth()
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

    // Helper lambda to refresh a wave selector
    auto refreshWave = [this](WaveSelector& selector, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            selector.setSelectedIndex(param->enumIndex, juce::dontSendNotification);
    };

    // Master
    refreshKnob(masterVolume, "master_volume");

    // OSC 1
    refreshWave(osc1Wave, "osc1_wave");
    refreshKnob(osc1Octave, "osc1_octave");
    refreshKnob(osc1Detune, "osc1_detune");

    // OSC 2
    refreshWave(osc2Wave, "osc2_wave");
    refreshKnob(osc2Octave, "osc2_octave");
    refreshKnob(osc2Detune, "osc2_detune");

    // FILTER
    refreshCombo(filterType, "filter_type");
    refreshKnob(filterCutoff, "filter_cutoff");
    refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterEnvAmount, "filter_env_amount");

    // AMP ENV
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // FILTER ENV
    refreshKnob(filterAttack, "filter_attack");
    refreshKnob(filterDecay, "filter_decay");
    refreshKnob(filterSustain, "filter_sustain");
    refreshKnob(filterRelease, "filter_release");

    // Update preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void AnalogSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    // Clear previous dividers
    sectionDividers.clear();

    const int labelHeight = 16;
    const int selectorHeight = 24;
    const int labelGap = 6;
    const int knobRowGap = 8;

    // Section widths: OSC1 (15%) | OSC2 (15%) | FILTER (20%) | AMP ENV (25%) | FLT ENV (25%)
    const int totalWidth = area.getWidth();
    const int osc1Width = totalWidth * 15 / 100;
    const int osc2Width = totalWidth * 15 / 100;
    const int filterWidth = totalWidth * 20 / 100;
    const int ampEnvWidth = totalWidth * 25 / 100;
    const int fltEnvWidth = totalWidth - osc1Width - osc2Width - filterWidth - ampEnvWidth;

    auto layoutSection = area;

    //==========================================================================
    // OSC 1 Section (15%)
    //==========================================================================
    {
        auto section = layoutSection.removeFromLeft(osc1Width).reduced(SECTION_PADDING, 0);

        // Label
        osc1Label.setBounds(section.removeFromTop(labelHeight));
        section.removeFromTop(labelGap);

        // Wave selector
        osc1Wave.setBounds(section.removeFromTop(selectorHeight));
        section.removeFromTop(knobRowGap);

        // Two knobs side by side
        auto knobRow = section.removeFromTop(KNOB_SIZE + 20); // Extra for label
        int knobSpacing = knobRow.getWidth() / 2;
        osc1Octave.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        osc1Detune.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
    }

    sectionDividers.push_back(layoutSection.getX());

    //==========================================================================
    // OSC 2 Section (15%)
    //==========================================================================
    {
        auto section = layoutSection.removeFromLeft(osc2Width).reduced(SECTION_PADDING, 0);

        // Label
        osc2Label.setBounds(section.removeFromTop(labelHeight));
        section.removeFromTop(labelGap);

        // Wave selector
        osc2Wave.setBounds(section.removeFromTop(selectorHeight));
        section.removeFromTop(knobRowGap);

        // Two knobs side by side
        auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
        int knobSpacing = knobRow.getWidth() / 2;
        osc2Octave.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        osc2Detune.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
    }

    sectionDividers.push_back(layoutSection.getX());

    //==========================================================================
    // FILTER Section (20%)
    //==========================================================================
    {
        auto section = layoutSection.removeFromLeft(filterWidth).reduced(SECTION_PADDING, 0);

        // Label
        filterLabel.setBounds(section.removeFromTop(labelHeight));
        section.removeFromTop(labelGap);

        // Filter type selector
        filterType.setBounds(section.removeFromTop(selectorHeight));
        section.removeFromTop(knobRowGap);

        // Three knobs
        auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
        int knobSpacing = knobRow.getWidth() / 3;
        filterCutoff.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        filterResonance.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        filterEnvAmount.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
    }

    sectionDividers.push_back(layoutSection.getX());

    //==========================================================================
    // AMP ENV Section (25%)
    //==========================================================================
    {
        auto section = layoutSection.removeFromLeft(ampEnvWidth).reduced(SECTION_PADDING, 0);

        // Label
        ampEnvLabel.setBounds(section.removeFromTop(labelHeight));
        section.removeFromTop(labelGap + selectorHeight + knobRowGap); // Align with other sections

        // Four knobs (ADSR)
        auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
        int knobSpacing = knobRow.getWidth() / 4;
        ampAttack.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        ampDecay.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        ampSustain.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        ampRelease.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
    }

    sectionDividers.push_back(layoutSection.getX());

    //==========================================================================
    // FLT ENV Section (25%)
    //==========================================================================
    {
        auto section = layoutSection.removeFromLeft(fltEnvWidth).reduced(SECTION_PADDING, 0);

        // Label
        filterEnvLabel.setBounds(section.removeFromTop(labelHeight));
        section.removeFromTop(labelGap + selectorHeight + knobRowGap); // Align with other sections

        // Four knobs (ADSR)
        auto knobRow = section.removeFromTop(KNOB_SIZE + 20);
        int knobSpacing = knobRow.getWidth() / 4;
        filterAttack.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        filterDecay.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        filterSustain.setBounds(knobRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
        filterRelease.setBounds(knobRow.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
    }
}

void AnalogSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw vertical dividers between sections
    for (int x : sectionDividers)
    {
        drawVerticalDivider(g, x, area.getY(), area.getBottom());
    }
}
