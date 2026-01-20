#include "ProSynthEditor.h"

ProSynthEditor::ProSynthEditor(ProSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Setup base class components
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "master_volume", "Volume");
    setupKnob(glideKnob, "glide", "Glide");

    // Initialize labels (will be positioned in layoutContent)
    osc1Label.setText("OSC 1", juce::dontSendNotification);
    osc2Label.setText("OSC 2", juce::dontSendNotification);
    osc3Label.setText("OSC 3", juce::dontSendNotification);
    subNoiseLabel.setText("SUB + NOISE", juce::dontSendNotification);
    filter1Label.setText("FILTER", juce::dontSendNotification);
    filterEnvLabel.setText("FILTER ENV", juce::dontSendNotification);
    ampEnvLabel.setText("AMP ENV", juce::dontSendNotification);
    unisonLabel.setText("UNISON", juce::dontSendNotification);

    // Oscillator 1
    setupComboBox(osc1Mode, "osc1_mode");
    setupComboBox(osc1Wave, "osc1_wave");
    setupKnob(osc1Level, "osc1_level", "Level");
    setupKnob(osc1Octave, "osc1_octave", "Oct");
    setupKnob(osc1Fine, "osc1_fine", "Fine", " ct");

    // Oscillator 2
    setupComboBox(osc2Mode, "osc2_mode");
    setupComboBox(osc2Wave, "osc2_wave");
    setupKnob(osc2Level, "osc2_level", "Level");
    setupKnob(osc2Octave, "osc2_octave", "Oct");
    setupKnob(osc2Fine, "osc2_fine", "Fine", " ct");

    // Oscillator 3
    setupComboBox(osc3Mode, "osc3_mode");
    setupComboBox(osc3Wave, "osc3_wave");
    setupKnob(osc3Level, "osc3_level", "Level");
    setupKnob(osc3Octave, "osc3_octave", "Oct");
    setupKnob(osc3Fine, "osc3_fine", "Fine", " ct");

    // Sub + Noise
    setupComboBox(subWave, "sub_wave");
    setupKnob(subLevel, "sub_level", "Sub Lvl");
    setupComboBox(noiseType, "noise_type");
    setupKnob(noiseLevel, "noise_level", "Noise");

    // Filter 1
    setupComboBox(filter1Model, "filter1_model");
    setupComboBox(filter1Type, "filter_type");
    setupKnob(filterCutoff, "filter_cutoff", "Cutoff", " Hz");
    setupKnob(filterResonance, "filter_resonance", "Res");
    setupKnob(filterDrive, "filter_drive", "Drive");

    // Filter Envelope
    setupKnob(filterEnvAttack, "filter_env_attack", "A", " s");
    setupKnob(filterEnvDecay, "filter_env_decay", "D", " s");
    setupKnob(filterEnvSustain, "filter_env_sustain", "S");
    setupKnob(filterEnvRelease, "filter_env_release", "R", " s");
    setupKnob(filterEnvAmount, "filter_env_amount", "Amt", " Hz");

    // Amp Envelope
    setupKnob(ampAttack, "amp_attack", "A", " s");
    setupKnob(ampDecay, "amp_decay", "D", " s");
    setupKnob(ampSustain, "amp_sustain", "S");
    setupKnob(ampRelease, "amp_release", "R", " s");

    // Unison
    setupKnob(unisonVoices, "unison_voices", "Voices");
    setupKnob(unisonDetune, "unison_detune", "Detune", " ct");

    refreshFromSynth();
}

ProSynthEditor::~ProSynthEditor()
{
    presetSelector.removeListener(this);
    osc1Mode.removeListener(this);
    osc1Wave.removeListener(this);
    osc2Mode.removeListener(this);
    osc2Wave.removeListener(this);
    osc3Mode.removeListener(this);
    osc3Wave.removeListener(this);
    subWave.removeListener(this);
    noiseType.removeListener(this);
    filter1Model.removeListener(this);
    filter1Type.removeListener(this);
}

void ProSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void ProSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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

void ProSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &osc1Mode) synth.setParameterEnum("osc1_mode", index);
    else if (box == &osc1Wave) synth.setParameterEnum("osc1_wave", index);
    else if (box == &osc2Mode) synth.setParameterEnum("osc2_mode", index);
    else if (box == &osc2Wave) synth.setParameterEnum("osc2_wave", index);
    else if (box == &osc3Mode) synth.setParameterEnum("osc3_mode", index);
    else if (box == &osc3Wave) synth.setParameterEnum("osc3_wave", index);
    else if (box == &subWave) synth.setParameterEnum("sub_wave", index);
    else if (box == &noiseType) synth.setParameterEnum("noise_type", index);
    else if (box == &filter1Model) synth.setParameterEnum("filter1_model", index);
    else if (box == &filter1Type) synth.setParameterEnum("filter_type", index);
}

void ProSynthEditor::populatePresets()
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

void ProSynthEditor::refreshFromSynth()
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
    refreshKnob(masterVolume, "master_volume");
    refreshKnob(glideKnob, "glide");

    // Oscillator 1
    refreshCombo(osc1Mode, "osc1_mode");
    refreshCombo(osc1Wave, "osc1_wave");
    refreshKnob(osc1Level, "osc1_level");
    refreshKnob(osc1Octave, "osc1_octave");
    refreshKnob(osc1Fine, "osc1_fine");

    // Oscillator 2
    refreshCombo(osc2Mode, "osc2_mode");
    refreshCombo(osc2Wave, "osc2_wave");
    refreshKnob(osc2Level, "osc2_level");
    refreshKnob(osc2Octave, "osc2_octave");
    refreshKnob(osc2Fine, "osc2_fine");

    // Oscillator 3
    refreshCombo(osc3Mode, "osc3_mode");
    refreshCombo(osc3Wave, "osc3_wave");
    refreshKnob(osc3Level, "osc3_level");
    refreshKnob(osc3Octave, "osc3_octave");
    refreshKnob(osc3Fine, "osc3_fine");

    // Sub + Noise
    refreshCombo(subWave, "sub_wave");
    refreshKnob(subLevel, "sub_level");
    refreshCombo(noiseType, "noise_type");
    refreshKnob(noiseLevel, "noise_level");

    // Filter
    refreshCombo(filter1Model, "filter1_model");
    refreshCombo(filter1Type, "filter_type");
    refreshKnob(filterCutoff, "filter_cutoff");
    refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterDrive, "filter_drive");

    // Filter Envelope
    refreshKnob(filterEnvAttack, "filter_env_attack");
    refreshKnob(filterEnvDecay, "filter_env_decay");
    refreshKnob(filterEnvSustain, "filter_env_sustain");
    refreshKnob(filterEnvRelease, "filter_env_release");
    refreshKnob(filterEnvAmount, "filter_env_amount");

    // Amp Envelope
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // Unison
    refreshKnob(unisonVoices, "unison_voices");
    refreshKnob(unisonDetune, "unison_detune");

    // Preset
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void ProSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int padding = SECTION_PADDING;
    const int comboHeight = 22;
    const int knobSpacing = 4;

    area.reduce(padding, padding);

    // Calculate section widths (8 sections: 3 OSCs + Sub/Noise + Filter + Filter Env + Amp Env + Unison)
    const int totalWidth = area.getWidth();
    const int sectionWidth = totalWidth / 8;

    sectionDividers.clear();

    auto layoutOscSection = [&](juce::Rectangle<int> section, juce::Label& label,
                                 juce::ComboBox& mode, juce::ComboBox& wave,
                                 RotaryKnob& level, RotaryKnob& octave, RotaryKnob& fine)
    {
        createSectionLabel(label, label.getText());
        addAndMakeVisible(label);
        label.setBounds(section.removeFromTop(16));
        section.removeFromTop(4);

        // Mode and Wave dropdowns
        auto comboRow = section.removeFromTop(comboHeight);
        mode.setBounds(comboRow.removeFromLeft(comboRow.getWidth() / 2 - 2));
        comboRow.removeFromLeft(4);
        wave.setBounds(comboRow);
        section.removeFromTop(knobSpacing);

        // Knobs in rows
        auto knobRow1 = section.removeFromTop(KNOB_SIZE);
        level.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));
        octave.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));

        section.removeFromTop(knobSpacing);
        auto knobRow2 = section.removeFromTop(KNOB_SIZE);
        fine.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));
    };

    // OSC 1
    auto osc1Area = area.removeFromLeft(sectionWidth);
    layoutOscSection(osc1Area, osc1Label, osc1Mode, osc1Wave, osc1Level, osc1Octave, osc1Fine);
    sectionDividers.push_back(area.getX());

    // OSC 2
    auto osc2Area = area.removeFromLeft(sectionWidth);
    layoutOscSection(osc2Area, osc2Label, osc2Mode, osc2Wave, osc2Level, osc2Octave, osc2Fine);
    sectionDividers.push_back(area.getX());

    // OSC 3
    auto osc3Area = area.removeFromLeft(sectionWidth);
    layoutOscSection(osc3Area, osc3Label, osc3Mode, osc3Wave, osc3Level, osc3Octave, osc3Fine);
    sectionDividers.push_back(area.getX());

    // SUB + NOISE
    {
        auto section = area.removeFromLeft(sectionWidth);
        createSectionLabel(subNoiseLabel, "SUB + NOISE");
        addAndMakeVisible(subNoiseLabel);
        subNoiseLabel.setBounds(section.removeFromTop(16));
        section.removeFromTop(4);

        auto comboRow1 = section.removeFromTop(comboHeight);
        subWave.setBounds(comboRow1);
        section.removeFromTop(knobSpacing);

        auto comboRow2 = section.removeFromTop(comboHeight);
        noiseType.setBounds(comboRow2);
        section.removeFromTop(knobSpacing);

        auto knobRow = section.removeFromTop(KNOB_SIZE);
        subLevel.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
        noiseLevel.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
    }
    sectionDividers.push_back(area.getX());

    // FILTER
    {
        auto section = area.removeFromLeft(sectionWidth);
        createSectionLabel(filter1Label, "FILTER");
        addAndMakeVisible(filter1Label);
        filter1Label.setBounds(section.removeFromTop(16));
        section.removeFromTop(4);

        auto comboRow1 = section.removeFromTop(comboHeight);
        filter1Model.setBounds(comboRow1);
        section.removeFromTop(knobSpacing);

        auto comboRow2 = section.removeFromTop(comboHeight);
        filter1Type.setBounds(comboRow2);
        section.removeFromTop(knobSpacing);

        auto knobRow = section.removeFromTop(KNOB_SIZE);
        filterCutoff.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
        filterResonance.setBounds(knobRow.removeFromLeft(KNOB_SIZE));

        section.removeFromTop(knobSpacing);
        auto knobRow2 = section.removeFromTop(KNOB_SIZE);
        filterDrive.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));
    }
    sectionDividers.push_back(area.getX());

    // FILTER ENVELOPE
    {
        auto section = area.removeFromLeft(sectionWidth);
        createSectionLabel(filterEnvLabel, "FILTER ENV");
        addAndMakeVisible(filterEnvLabel);
        filterEnvLabel.setBounds(section.removeFromTop(16));
        section.removeFromTop(4);

        auto knobRow1 = section.removeFromTop(KNOB_SIZE);
        filterEnvAttack.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));
        filterEnvDecay.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));

        section.removeFromTop(knobSpacing);
        auto knobRow2 = section.removeFromTop(KNOB_SIZE);
        filterEnvSustain.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));
        filterEnvRelease.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));

        section.removeFromTop(knobSpacing);
        auto knobRow3 = section.removeFromTop(KNOB_SIZE);
        filterEnvAmount.setBounds(knobRow3.removeFromLeft(KNOB_SIZE));
    }
    sectionDividers.push_back(area.getX());

    // AMP ENVELOPE
    {
        auto section = area.removeFromLeft(sectionWidth);
        createSectionLabel(ampEnvLabel, "AMP ENV");
        addAndMakeVisible(ampEnvLabel);
        ampEnvLabel.setBounds(section.removeFromTop(16));
        section.removeFromTop(4);

        auto knobRow1 = section.removeFromTop(KNOB_SIZE);
        ampAttack.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));
        ampDecay.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));

        section.removeFromTop(knobSpacing);
        auto knobRow2 = section.removeFromTop(KNOB_SIZE);
        ampSustain.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));
        ampRelease.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));
    }
    sectionDividers.push_back(area.getX());

    // UNISON
    {
        auto section = area;
        createSectionLabel(unisonLabel, "UNISON");
        addAndMakeVisible(unisonLabel);
        unisonLabel.setBounds(section.removeFromTop(16));
        section.removeFromTop(4);

        auto knobRow = section.removeFromTop(KNOB_SIZE);
        unisonVoices.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
        unisonDetune.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
    }
}

void ProSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    for (int x : sectionDividers)
    {
        drawVerticalDivider(g, x, area.getY(), area.getBottom());
    }
}
